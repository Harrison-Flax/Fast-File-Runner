#include <iostream>
#include <string>
#include <curl/curl.h>
#include <zip.h>
#include "rapidjson/document.h"

// This file will load and extract the dataset from Kaggle
// API will be accessed from Kaggle directory on local machine
// With Kaggle CLI installed
// Utilizing libcurl + libzip + RapidJSON Parsing + C++ Concurrency

// Reference: https://gist.github.com/alghanmi/c5d7b761b2c9ab199157#file-curl_example-cpp
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	size_t totalSize = size * nmemb;

	// Storing in a string for the data
	std::string* reponseString = static_cast<std::string*>(userp);
	responseString->append(static_cast<char*>(contents), totalSize);
	return totalSize;
}

// Reference: https://libzip.org/documentation/
void DecompressZipFromMemory(const std::string& zipData) {
	zip_error_t error;
	zip_error_init(&error);

	// Libzip source is created
	zip_source_t* src = zip_source_buffer_create(zipData.data(), zipData.size(), 0, &error);

	// Zip is opened
	zip_t* archive = zip_open_from_source(src, ZIP_RDONLY, &error);

	// Entries in zip
	zip_int64_t numEntries = zip_get_num_entries(archive, 0);
	std::cout << "Total files in ZIP: " << numEntries << std::endl;

	// Extracting each JSON file
	for (zip_int64_t i = 0; i < numEntries; ++i) {
		const char* name = zip_get_name(archive, i, 0);
		if (!name) continue;

		std::cout << "Extracting: " << name << std::endl;

		// Open each of the JSON files in the ZIP
		zip_file_t* file = zip_fopen_index(archive, i, 0);

		// Listing the file information for uncompressed size
		zip_stat_t stat;
		if (zip_stat_index(archive, i, 0, &stat) == 0 && (stat.valid & ZIP_STAT_SIZE)) {
			std::vector<char> buffer(stat.size);
			zip_int64_t bytesRead = zip_fread(file, buffer.data(), stat.size);

			std::cout << "Read " << bytesRead << " bytes!" << std::endl;
		}

		// Closing the zip once done
		zip_fclose(file);
	}

	// Cleaning up functions in memory
	zip_close(archive);
	zip_error_fini(&error);
}

int main() {
	CURL* curl;
	CURLcode res; 
	std::string readBuffer;
	rapidjson::Document document;

	// Reference: https://curl.se/libcurl/c/libcurl-tutorial.html
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		// Kaggle URL from dataset
		curl_easy_setopt(curl, CURLOPT_URL, "https://www.kaggle.com/api/v1/datasets/download/jeet2016/us-financial-news-articles");
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		// Written to callback function and buffer memory address
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// Execution of request over network
		res = curl_easy_perform(curl);

		// Transfer check
		if (res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		} else {
			std::cout << "Download Complete! Size: " << readBuffer.size() << " bytes." << std::endl;
			// Unzip
			DecompressZipFromMemory(readBuffer);
			// Parse it
			document.Parse(readBuffer)
		}

		// Cleaning up easy resources
		curl_easy_cleanup(curl);
	}

	// Cleaning uo global resources
	curl_global_cleanup();
	return 0;
}