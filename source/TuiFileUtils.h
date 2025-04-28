
#ifndef __TuiFileUtils__
#define __TuiFileUtils__

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include "TuiLog.h"

namespace Tui {

std::string getFileContents(const std::string& filename);
void writeToFile(const std::string& filename, const std::string& data);

std::string getResourcePath(const std::string &appendPath = "");
std::string getSavePath(const std::string &appendPath = "");

std::vector<std::string> getDirectoryContents(const std::string& dirName);
std::string fileNameFromPath(const std::string& path);
std::string fileExtensionFromPath(const std::string& path);
std::string changeExtensionForPath(const std::string& path, const std::string& newExtension);
std::string removeExtensionForPath(const std::string& path);
std::string pathByRemovingLastPathComponent(const std::string& path);
std::string pathByAppendingPathComponent(const std::string& path, const std::string& appendPath);
std::string normalizedPath(const std::string& path);

int64_t fileSizeAtPath(const std::string& path);
bool fileExistsAtPath(const std::string& path);
bool isSymLinkAtPath(const std::string& path);
bool isDirectoryAtPath(const std::string& path);

void createDirectoriesIfNeededForDirPath(const std::string& path);
void createDirectoriesIfNeededForFilePath(const std::string& path);

void moveFile(const std::string& fromPath, const std::string& toPath);
bool removeFile(const std::string& removePath);
bool removeEmptyDirectory(const std::string& removePath);
bool removeDirectory(const std::string& removePath);

bool copyFileOrDir(const std::string& sourcePath, const std::string& destinationPath);

void openFile(std::string filePath);

}

#endif
