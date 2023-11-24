#include <gfs/gfs.hpp>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

std::random_device sRandDevice;
std::mt19937 sRandGen(sRandDevice());
std::uniform_real_distribution<float> sFloatDist;
std::uniform_int_distribution<uint32_t> sIntDist;

constexpr uint32_t TEST_DATA_COUNT = 50000;
constexpr uint32_t TEST_LOOP_COUNT = 20;
constexpr const char* TEST_TMP_FILE_NAME = "tmp.bin";

static auto Benchmark(uint32_t loopTimes, const std::function<void()>& func) -> uint32_t
{
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;
	using clock = std::chrono::high_resolution_clock;

	auto start = clock::now();

	for (auto i = 0u; i < loopTimes; ++i)
		func();

	auto end = clock::now();
	auto duration = uint32_t(duration_cast<milliseconds>(end - start).count());

	if (std::filesystem::exists(TEST_TMP_FILE_NAME))
		std::filesystem::remove(TEST_TMP_FILE_NAME);

	return duration;
}

struct DataObject
{
	float floats[4]{};
	uint32_t ints[4]{};
	bool b;

	DataObject()
	{
		for (auto& v : floats)
			v = sFloatDist(sRandGen);
		for (auto& v : ints)
			v = sIntDist(sRandGen);
		b = false;
	}
};

int main()
{
	std::cout << "GFS Benchmark" << std::endl;

	std::cout << "Generating test data..." << std::endl;
	std::vector<DataObject> testData(TEST_DATA_COUNT);
	std::cout << "Data Size = " << sizeof(DataObject) * TEST_DATA_COUNT << "bytes" << std::endl;
	std::cout << "Loop Count = " << TEST_LOOP_COUNT << std::endl;

	uint32_t duration = 0;

	duration = Benchmark(TEST_LOOP_COUNT, [testData]() {
		std::ofstream tmpStream(TEST_TMP_FILE_NAME, std::ios::binary);
		for (const auto& value : testData)
		{
			tmpStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
		}
	});
	std::cout << "Write straight to file (iterate each value): " << duration << "ms" << std::endl;

	duration = Benchmark(TEST_LOOP_COUNT, [testData]() {
		std::ofstream tmpStream(TEST_TMP_FILE_NAME, std::ios::binary);
		tmpStream.write(reinterpret_cast<const char*>(testData.data()), sizeof(DataObject) * testData.size());
	});
	std::cout << "Write straight to file (write whole vector assuming simple POD type): " << duration << "ms" << std::endl;

	duration = Benchmark(TEST_LOOP_COUNT, [testData]() {
		std::vector<uint8_t> binaryData;
		for (const auto& value : testData)
		{
			const auto i = binaryData.size();
			binaryData.resize(binaryData.size() + sizeof(DataObject));
			std::memcpy(&binaryData[i], &value, sizeof(DataObject));
		}
		std::ofstream tmpStream(TEST_TMP_FILE_NAME, std::ios::binary);
		tmpStream.write(reinterpret_cast<const char*>(binaryData.data()), binaryData.size());
	});
	std::cout << "Vector buffer: " << duration << "ms" << std::endl;

	duration = Benchmark(TEST_LOOP_COUNT, [testData]() {
		std::vector<uint8_t> binaryData;
		binaryData.reserve(sizeof(DataObject) * TEST_DATA_COUNT);
		for (const auto& value : testData)
		{
			const auto i = binaryData.size();
			binaryData.resize(binaryData.size() + sizeof(DataObject));
			std::memcpy(&binaryData[i], &value, sizeof(DataObject));
		}
		std::ofstream tmpStream(TEST_TMP_FILE_NAME, std::ios::binary);
		tmpStream.write(reinterpret_cast<const char*>(binaryData.data()), binaryData.size());
	});
	std::cout << "Vector buffer (Preallocated): " << duration << "ms" << std::endl;

	return 0;
}