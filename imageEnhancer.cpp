#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>

using std::cout;
using std::endl;
using std::string;
using std::vector;

struct Image {
	int w, h, c;
	vector<float> data; // RGB in [0,1]
	Image() : w(0), h(0), c(0) {}
};

static inline float clamp01(float v) {
	return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static inline void toLowerInPlace(string& s) {
	for (size_t i = 0; i < s.size(); ++i) {
		char ch = s[i];
		if (ch >= 'A' && ch <= 'Z') s[i] = (char)(ch - 'A' + 'a');
	}
}

bool loadImage(const string& path, Image& img) {
	int w, h, n;
	unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &n, 3);
	if (!pixels) return false;
	img.w = w; img.h = h; img.c = 3;
	img.data.resize(w * h * 3);
	for (int i = 0; i < w * h * 3; ++i) img.data[i] = pixels[i] / 255.0f;
	stbi_image_free(pixels);
	return true;
}

bool saveImage(const string& path, const Image& img) {
	vector<unsigned char> out(img.w * img.h * 3);
	for (int i = 0; i < img.w * img.h * 3; ++i) {
		float v = clamp01(img.data[i]);
		out[i] = (unsigned char)std::lround(v * 255.0f);
	}
	// extension
	string ext;
	size_t dot = path.find_last_of('.');
	if (dot != string::npos) ext = path.substr(dot);
	toLowerInPlace(ext);

	if (ext == ".png") {
		return stbi_write_png(path.c_str(), img.w, img.h, 3, out.data(), img.w * 3) != 0;
	} else if (ext == ".jpg" || ext == ".jpeg") {
		return stbi_write_jpg(path.c_str(), img.w, img.h, 3, out.data(), 95) != 0;
	} else if (ext == ".bmp") {
		return stbi_write_bmp(path.c_str(), img.w, img.h, 3, out.data()) != 0;
	} else {
		// default to PNG if unknown ext
		return stbi_write_png(path.c_str(), img.w, img.h, 3, out.data(), img.w * 3) != 0;
	}
}

vector<float> gaussianKernel1D(int radius, float sigma) {
	int size = 2 * radius + 1;
	vector<float> k(size);
	float sum = 0.0f;
	float inv2s2 = 1.0f / (2.0f * sigma * sigma);
	for (int i = -radius; i <= radius; ++i) {
		float v = std::exp(-i * i * inv2s2);
		k[i + radius] = v;
		sum += v;
	}
	for (int i = 0; i < size; ++i) k[i] /= sum;
	return k;
}

void gaussianBlurRGB(const Image& src, Image& dst, int radius, float sigma) {
	dst = src;
	vector<float> k = gaussianKernel1D(radius, sigma);
	vector<float> tmp(src.w * src.h * 3, 0.0f);

	// Horizontal
	for (int y = 0; y < src.h; ++y) {
		for (int x = 0; x < src.w; ++x) {
			for (int ch = 0; ch < 3; ++ch) {
				float acc = 0.0f;
				for (int i = -radius; i <= radius; ++i) {
					int xx = x + i;
					if (xx < 0) xx = 0;
					if (xx >= src.w) xx = src.w - 1;
					acc += src.data[(y * src.w + xx) * 3 + ch] * k[i + radius];
				}
				tmp[(y * src.w + x) * 3 + ch] = acc;
			}
		}
	}

	// Vertical
	for (int y = 0; y < src.h; ++y) {
		for (int x = 0; x < src.w; ++x) {
			for (int ch = 0; ch < 3; ++ch) {
				float acc = 0.0f;
				for (int i = -radius; i <= radius; ++i) {
					int yy = y + i;
					if (yy < 0) yy = 0;
					if (yy >= src.h) yy = src.h - 1;
					acc += tmp[(yy * src.w + x) * 3 + ch] * k[i + radius];
				}
				dst.data[(y * src.w + x) * 3 + ch] = acc;
			}
		}
	}
}

void unsharpMask(Image& img, float sigma, int radius, float amount) {
	Image blurred;
	gaussianBlurRGB(img, blurred, radius, sigma);
	int N = img.w * img.h * 3;
	for (int i = 0; i < N; ++i) {
		float detail = img.data[i] - blurred.data[i];
		img.data[i] = clamp01(img.data[i] + amount * detail);
	}
}

static inline float rgb2y(float r, float g, float b) {
	return 0.299f * r + 0.587f * g + 0.114f * b;
}

void equalizeLuminance(Image& img) {
	const int bins = 256;
	vector<int> hist(bins, 0);
	vector<float> Y(img.w * img.h);

	for (int i = 0; i < img.w * img.h; ++i) {
		float r = img.data[i * 3 + 0];
		float g = img.data[i * 3 + 1];
		float b = img.data[i * 3 + 2];
		float y = rgb2y(r, g, b);
		Y[i] = y;
		int bin = (int)std::lround(clamp01(y) * 255.0f);
		++hist[bin];
	}

	vector<float> cdf(bins, 0.0f);
	int total = img.w * img.h;
	int cum = 0;
	for (int i = 0; i < bins; ++i) {
		cum += hist[i];
		cdf[i] = (float)cum / (float)total;
	}

	const float eps = 1e-6f;
	for (int i = 0; i < img.w * img.h; ++i) {
		int bin = (int)std::lround(clamp01(Y[i]) * 255.0f);
		float yNew = cdf[bin];
		float scale = yNew / ((Y[i] > eps) ? Y[i] : (eps));

		float r = img.data[i * 3 + 0] * scale;
		float g = img.data[i * 3 + 1] * scale;
		float b = img.data[i * 3 + 2] * scale;

		img.data[i * 3 + 0] = clamp01(r);
		img.data[i * 3 + 1] = clamp01(g);
		img.data[i * 3 + 2] = clamp01(b);
	}
}

Image enhanceFace(const Image& input) {
	Image work = input;
	unsharpMask(work, 1.3f, 2, 1.1f);
	equalizeLuminance(work);
	unsharpMask(work, 0.9f, 1, 0.4f);
	return work;
}

// Ensure output directory exists (best-effort)
static void ensureOutputDir() {
	CreateDirectoryA("output", NULL); // succeeds or already exists
}

static bool hasSupportedExt(const string& filename) {
	size_t dot = filename.find_last_of('.');
	if (dot == string::npos) return false;
	string ext = filename.substr(dot);
	toLowerInPlace(ext);
	return (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp");
}

int main() {
	string inputDir = "images\\";
	string outputDir = "output\\";

	ensureOutputDir();

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	string search = inputDir + "*";
	hFind = FindFirstFileA(search.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		cout << "Could not open images directory." << endl;
		return 1;
	}

	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue; // skip subdirectories
		}

		string fname = ffd.cFileName;
		if (!hasSupportedExt(fname)) continue;

		string imagePath = inputDir + fname;
		Image img;
		if (!loadImage(imagePath, img)) {
			cout << "Could not open image: " << imagePath << endl;
			continue;
		}

		cout << "Enhancing: " << imagePath << endl;
		Image enhanced = enhanceFace(img);

		string outPath = outputDir + fname;
		if (!saveImage(outPath, enhanced)) {
			cout << "Failed to save: " << outPath << endl;
		} else {
			cout << "Saved enhanced image to: " << outPath << endl;
		}

	} while (FindNextFileA(hFind, &ffd) != 0);

	FindClose(hFind);

	cout << "All images processed successfully!" << endl;
	return 0;
}