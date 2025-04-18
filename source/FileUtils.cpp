#include "FileUtils.h"
#include "StringUtils.h"

#include <fstream>

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

std::string getFileContents(const std::string& filename)
{
    std::ifstream in((filename).c_str(), std::ios::in | std::ios::binary);
	if(!in)
	{
		std::string tmpFilename = filename + ".tmp";
		if(fileExistsAtPath(tmpFilename))
		{
			moveFile(tmpFilename, filename);
			in = std::ifstream((filename).c_str(), std::ios::in | std::ios::binary);
		}
	}
    if(in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return(contents);
    }
    
    return "";
}

void writeToFile(const std::string& filename, const std::string& data)
{
	std::string tmpFile = filename + ".tmp";
	std::ofstream ofs((tmpFile).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
	if(ofs)
	{
		ofs.write(data.data(), data.size());
		ofs.close();
		moveFile(tmpFile, filename);
	}
}

void moveFile(const std::string& fromPath, const std::string& toPath)
{
#ifdef WIN32 //windows won't overwrite, doesn't seem to be a way to do it atomically, so just remove then rename.
	_wremove(convertUtf8ToWide(toPath).c_str());
	_wrename(convertUtf8ToWide(fromPath).c_str(), convertUtf8ToWide(toPath).c_str());
#else
	rename(fromPath.c_str(), toPath.c_str());
#endif
}

bool removeFile(const std::string& removePath)
{
#ifdef WIN32
	return (_wremove(convertUtf8ToWide(removePath).c_str()) == 0);
#else
	return (remove(removePath.c_str()) == 0);
#endif
}


bool removeEmptyDirectory(const std::string& removePath)
{
#ifdef WIN32
	return (_wrmdir(convertUtf8ToWide(removePath).c_str()) == 0);
#else
	//return (_rmdir(removePath.c_str()) == 0); //maybe linux?
    return (rmdir(removePath.c_str()) == 0);
#endif
}

bool removeDirectory(const std::string& removePath)
{
	std::vector<std::string> contents = getDirectoryContents(removePath);
	for(auto& subName : contents)
	{
		std::string subPath = removePath + "/" + subName;
		if(isDirectoryAtPath(subPath) && !isSymLinkAtPath(subPath))
		{
			removeDirectory(subPath);
		}
		else
		{
			removeFile(subPath);
		}
	}
	return removeEmptyDirectory(removePath);
}


std::string getResourcePath(const std::string &appendPath)
{
    return appendPath;
    /*
    static std::string basePath;
    if(basePath.empty())
    {
        char *basePathCString = SDL_GetBasePath();
        if (basePathCString){
            basePath = basePathCString;
            basePath = basePath + "resources/";
            SDL_free(basePathCString);
        }
        else {
            std::cerr << "Error getting resource path: " << SDL_GetError() << std::endl;
            return "";
        }
    }

    if(appendPath.empty())
    {
        return basePath;
    }

    return basePath + appendPath;*/
}

std::vector<std::string> getDirectoryContents(const std::string& dirName)
{
    std::vector<std::string> result;
#ifdef WIN32
    WIN32_FIND_DATAW findData;
    std::string query = dirName + "/*";
    HANDLE findHandle = FindFirstFileW(convertUtf8ToWide(query).c_str(), &findData);

    if (findHandle == INVALID_HANDLE_VALUE)
    {
        return result;
    }

    do
    {
        std::string name = convertWideToUtf8(findData.cFileName);
        if (name != ".." && name != ".")
        {
            result.push_back(name);
        }
    } while (FindNextFileW(findHandle, &findData));

    FindClose(findHandle);
#else
    DIR * dir = opendir(dirName.c_str());
    if (!dir)
    {
        return result;
    }

    // Read directory entries
    struct dirent * entry = readdir(dir);
    while (entry)
    {
        // Get name
        std::string name = entry->d_name;

        // Ignore . and ..
        if (name != ".." && name != ".")
        {
            result.push_back(name);
        }

        // Next entry
        entry = readdir(dir);
    }

    // Close directory
    closedir(dir);
#endif
    
    return result;
}

std::string fileNameFromPath(const std::string& path)
{
    std::vector<std::string> filePathComponents = splitString(normalizedPath(path), '/');
    if(filePathComponents.size() > 0)
    {
        return filePathComponents[filePathComponents.size() - 1];  
    }
    MJLog("Error finding fileNameFromPath for input path:%s", path.c_str());
    return "";
}

std::string fileExtensionFromPath(const std::string& path)
{
    std::string fileName = fileNameFromPath(path);
    if(!fileName.empty())
    {
        std::vector<std::string> components = splitString(fileName, '.');
        if(components.size() > 0)
        {
            return "." + components[components.size() - 1];
        }
    }
    MJLog("Error finding fileExtensionFromPath for input path:%s", path.c_str());
    return "";
}

std::string changeExtensionForPath(const std::string& path, const std::string& newExtension)
{
    std::vector<std::string> filePathComponents = splitString(normalizedPath(path), '/');
    if(filePathComponents.size() > 0)
    {
        std::vector<std::string> fileNameComponents = splitString(filePathComponents[filePathComponents.size() - 1], '.');
        if(fileNameComponents.size() > 0)
        {
            std::string result = "";
            for(int i = 0; i < filePathComponents.size() - 1; i++)
            {
                result = result + filePathComponents[i] + "/";
            }

            if(newExtension[0] == '.')
            {
                result = result + fileNameComponents[0] + newExtension;
            }
            else
            {
                result = result + fileNameComponents[0] + "." + newExtension;
            }

            return result;  
        }
    }
    MJLog("Error in changeExtensionForPath for input path:%s", path.c_str());
    return "";
}

std::string removeExtensionForPath(const std::string& path)
{
	std::vector<std::string> filePathComponents = splitString(normalizedPath(path), '/');
	if(filePathComponents.size() > 0)
	{
		std::vector<std::string> fileNameComponents = splitString(filePathComponents[filePathComponents.size() - 1], '.');
		if(fileNameComponents.size() > 0)
		{
			std::string result = "";
			for(int i = 0; i < filePathComponents.size() - 1; i++)
			{
				result = result + filePathComponents[i] + "/";
			}

			result = result + fileNameComponents[0];

			return result;  
		}
	}
	MJLog("Error in changeExtensionForPath for input path:%s", path.c_str());
	return "";
}

std::string pathByRemovingLastPathComponent(const std::string& path)
{
    std::vector<std::string> filePathComponents = splitString(normalizedPath(path), '/');
    if(filePathComponents.size() > 0)
    {
        std::string result = "";
        for(int i = 0; i < filePathComponents.size() - 1; i++)
        {
            result = result + filePathComponents[i] + "/";
        }

        return result;
    }
    MJLog("Error pathByRemovingLastPathComponent for input path:%s", path.c_str());
    return "";
}

std::string pathByAppendingPathComponent(const std::string& path, const std::string& appendPath)
{
    std::string outPath = normalizedPath(path);
    if(!outPath.empty())
    {
        if(outPath[outPath.length() - 1] != '/')
        {
            outPath = outPath + "/";
        }

        outPath = outPath + appendPath;
        return outPath;
    }
    MJLog("Error pathByAppendingPathComponent for input path:%s", path.c_str());
    return "";
}

std::string normalizedPath(const std::string& path)
{
    return stringByReplacingString(stringByReplacingString(path, "\\", "/"), "//", "/");
}

int64_t fileSizeAtPath(const std::string& path)
{
#ifdef WIN32
    WIN32_FILE_ATTRIBUTE_DATA* m_fileInfo = new WIN32_FILE_ATTRIBUTE_DATA;

    if (!GetFileAttributesExW(convertUtf8ToWide(path).c_str(), GetFileExInfoStandard, m_fileInfo))
    {
        delete m_fileInfo;
        m_fileInfo = nullptr;
    }

    if(m_fileInfo)
    {
        auto fileSizeH  = m_fileInfo->nFileSizeHigh;
        auto fileSizeL = m_fileInfo->nFileSizeLow;
		delete m_fileInfo;
        return static_cast<int64_t>(static_cast<__int64>(fileSizeH) << 32 | fileSizeL);
    }
#else
    struct stat* m_fileInfo = new struct stat;

    // Get file info
    if (stat(path.c_str(), m_fileInfo) != 0)
    {
        // Error!
        delete m_fileInfo;
        m_fileInfo = nullptr;
    }

    if (m_fileInfo)
    {
        if (S_ISREG( m_fileInfo->st_mode ))
        {
            return m_fileInfo->st_size;
        }
        else
        {
            return 0;
        }
    }
#endif
    
    return -1;
}

bool isSymLinkAtPath(const std::string& path)
{
#ifdef WIN32
	WIN32_FILE_ATTRIBUTE_DATA* m_fileInfo = new WIN32_FILE_ATTRIBUTE_DATA;

	if (!GetFileAttributesExW(convertUtf8ToWide(path).c_str(), GetFileExInfoStandard, m_fileInfo))
	{
		delete m_fileInfo;
		m_fileInfo = nullptr;
	}

	if(m_fileInfo)
	{
		return m_fileInfo->dwFileAttributes != INVALID_FILE_ATTRIBUTES && (m_fileInfo->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
	}
	
#else
    struct stat p_statbuf;

    if (lstat(path.c_str(), &p_statbuf) < 0) {  /* if error occured */
        return false;
    }

    if (S_ISLNK(p_statbuf.st_mode) == 1) {
        return true;
    }
#endif

	return false;
}

bool isDirectoryAtPath(const std::string& path)
{
#ifdef WIN32
	WIN32_FILE_ATTRIBUTE_DATA* m_fileInfo = new WIN32_FILE_ATTRIBUTE_DATA;

	if (!GetFileAttributesExW(convertUtf8ToWide(path).c_str(), GetFileExInfoStandard, m_fileInfo))
	{
		delete m_fileInfo;
		m_fileInfo = nullptr;
	}

	if(m_fileInfo)
	{
		return m_fileInfo->dwFileAttributes != INVALID_FILE_ATTRIBUTES && (m_fileInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	}
#else
	struct stat* m_fileInfo = new struct stat;

	// Get file info
	if (stat(path.c_str(), m_fileInfo) != 0)
	{
		// Error!
		delete m_fileInfo;
		m_fileInfo = nullptr;
	}

	if (m_fileInfo)
	{
		return S_ISDIR(m_fileInfo->st_mode);
	}
#endif

	return false;
}

bool fileExistsAtPath(const std::string& path)
{
#ifdef WIN32
    WIN32_FILE_ATTRIBUTE_DATA* m_fileInfo = new WIN32_FILE_ATTRIBUTE_DATA;

    if (!GetFileAttributesExW(convertUtf8ToWide(path).c_str(), GetFileExInfoStandard, m_fileInfo))
    {
        delete m_fileInfo;
        m_fileInfo = nullptr;
    }

    if(m_fileInfo)
    {
        return true;
    }
#else
    struct stat* m_fileInfo = new struct stat;

    // Get file info
    if (stat(path.c_str(), m_fileInfo) != 0)
    {
        // Error!
        delete m_fileInfo;
        return false;
    }

    if (m_fileInfo)
    {
        delete m_fileInfo;
        return true;
    }
#endif

    return false;
}

static std::string basePath;

void setFileSaveBasePath(const std::string& newBasePath)
{
    if(newBasePath.empty())
    {
        return;
    }
    
    basePath = newBasePath;
    if(basePath[basePath.length() - 1] != '/')
    {
        basePath = basePath + "/";
    }
    
    if(!(fileExistsAtPath(basePath)))
    {
        std::cerr << "Error, the specified game save directory does not exist. Please create it first:" << basePath << std::endl;
        exit(0);
    }
    else
    {
        basePath = basePath + "ambience/";
        if(!(fileExistsAtPath(basePath)))
        {
            createDirectoriesIfNeededForDirPath(basePath);
        }
    }
}

std::string getSavePath(const std::string& appendPath)
{
    return appendPath;
    /*
    if(basePath.empty())
    {
        char *basePathCString = SDL_GetPrefPath("majicjungle", "ambience");
        if (basePathCString){
            basePath = basePathCString;
            SDL_free(basePathCString);
        }
        else {
            std::cerr << "Error getting save path: " << SDL_GetError() << std::endl;
            return "";
        }
    }

    if(appendPath.empty())
    {
        return basePath;
    }

    return basePath + appendPath;*/
}


std::string getWorldSavePath(const std::string& playerID, const std::string& worldID, const std::string& appendPath)
{
    std::string worldsDir = getSavePath("players/" + playerID + "/worlds/" + worldID);
    
    if(appendPath.empty())
    {
        return worldsDir;
    }
    
    return worldsDir + "/" + appendPath;
}

std::string getScriptPath(const std::string &appendPath)
{
//#if (DEBUG || HOTLOAD_DEVELOPMENT)
//    std::string basePath = DEV_RESOURCES_DIR;
//    basePath = pathByAppendingPathComponent(basePath, "scripts/");
//#else
    std::string basePath = getResourcePath("scripts/");
//#endif
    if(appendPath.empty())
    {
        return basePath;
    }

    return pathByAppendingPathComponent(basePath, appendPath);
}

void createDirectoriesIfNeededForDirPath(const std::string& path)
{
    std::vector<std::string> filePathComponents = splitString(normalizedPath(path), '/');
    if(filePathComponents.size() > 0)
    {
        std::string thisPath = filePathComponents[0];
        for(int i = 1; i < filePathComponents.size(); i++)
        {
            thisPath = thisPath + "/" + filePathComponents[i];
            if(!fileExistsAtPath(thisPath))
            {
                MJLog("mkdir:%s", thisPath.c_str());
                int error = 0;
#ifdef WIN32
                error = _wmkdir(convertUtf8ToWide(thisPath).c_str());
#else
                mode_t nMode = 0755;
                error = mkdir(thisPath.c_str(),nMode);
#endif
                if(error)
                {
                    MJLog("Error createDirectoriesIfNeededForDirPath for input path:%s", path.c_str());
                }
            }
        }
    }
}

void createDirectoriesIfNeededForFilePath(const std::string& path)
{
    std::string dirPath = pathByRemovingLastPathComponent(path);
    createDirectoriesIfNeededForDirPath(dirPath);
}

/*
bool copyFile (const string &src, const string &dest, bool remove_src) {
	ifstream ifs(src, ios::in|ios::binary);
	ofstream ofs(dest, ios::out|ios::binary);
	if (ifs && ofs) {
		ofs << ifs.rdbuf();
		if (!ifs.fail() && !ofs.fail() && remove_src) {
			ofs.close();
			ifs.close();
			return remove(src);
		}
		else
			return !remove_src;
	}
	return false;
}*/

bool copyFile(const std::string& sourcePath, const std::string& destinationPath)
{
	//MJLog("copyFile:%s -> %s", sourcePath.c_str(), destinationPath.c_str());
	std::ifstream ifs((sourcePath).c_str(), std::ios::in|std::ios::binary);
	std::ofstream ofs((destinationPath).c_str(), std::ios::out|std::ios::binary);
	if(ifs && ofs)
	{
		ofs << ifs.rdbuf();
		ifs.close();
		ofs.close();
		return true;
	}

	MJLog("ERROR: Failed to copy file %s -> %s", sourcePath.c_str(), destinationPath.c_str());

	return false;
}

bool copyDirectory(const std::string& sourcePath, const std::string& destinationPath)
{
	createDirectoriesIfNeededForDirPath(destinationPath);
	std::vector<std::string> contents = getDirectoryContents(sourcePath);
	for(const std::string& subFile : contents)
	{
		std::string fullSourcePath = pathByAppendingPathComponent(sourcePath, subFile);
		std::string fullDstPath = pathByAppendingPathComponent(destinationPath, subFile);
		if(isDirectoryAtPath(fullSourcePath) && !isSymLinkAtPath(fullSourcePath))
		{
			if(!copyDirectory(fullSourcePath, fullDstPath))
			{
				return false;
			}
		}
		else
		{ 
			if(!copyFile(fullSourcePath, fullDstPath))
			{
				return false;
			}
		}
	}
	return true;
}

bool copyFileOrDir(const std::string& sourcePath, const std::string& destinationPath)
{
	//MJLog("copyFileOrDir:%s -> %s", sourcePath.c_str(), destinationPath.c_str());

	if(fileExistsAtPath(sourcePath))
	{
		if(isDirectoryAtPath(sourcePath) && !isSymLinkAtPath(sourcePath))
		{
			return copyDirectory(sourcePath, destinationPath);
		}
		else
		{
			return copyFile(sourcePath, destinationPath);
		}
	}
	else
	{
		MJLog("WARNING: Source file or directory not found for copyFileOrDir:%s", sourcePath.c_str());
		return false;
	}
	return true;
}
/*
bool addFilesRecursively(const std::string& startPath, const std::string& thisPath, zip_t *zipper)
{
	std::vector<std::string> contents = getDirectoryContents(thisPath);
	for(const std::string& subFile : contents)
	{
		std::string fullPath = pathByAppendingPathComponent(thisPath, subFile); 
		if(!isSymLinkAtPath(fullPath))
		{
			if(isDirectoryAtPath(fullPath))
			{
				if(zip_dir_add(zipper, fullPath.substr(startPath.length() + 1).c_str(), ZIP_FL_ENC_UTF_8 ) == -1)
				{
					MJLog("ERROR: Failed to add dir to zip archive:%s", fullPath.c_str());
					return false;
				}
				if(!addFilesRecursively(startPath, fullPath, zipper))
				{
					return false;
				}
			}
			else
			{
				zip_source_t *source = zip_source_file(zipper, fullPath.c_str(), 0, 0);
				if(!source)
				{
					MJLog("ERROR: Failed to compress file:%s", fullPath.c_str());
					return false;
				}
				else
				{
					if(zip_file_add(zipper, fullPath.substr(startPath.length() + 1).c_str(), source, ZIP_FL_ENC_UTF_8) == -1)
					{
						MJLog("ERROR: Failed to add file to zip archive:%s", fullPath.c_str());
						//maybe should free, safer to maybe leak assuming this doesn't happen often
						return false;
					}
				}
			}
		}
	}
	return true;
}

bool zipDirectory(const std::string& dirPath, const std::string& archivePath)
{
	//MJLog("zipDirectory:%s->%s", dirPath, archivePath);

	if(fileExistsAtPath(dirPath))
	{
		if(isDirectoryAtPath(dirPath))
		{
			int errorp;
			zip_t *zipper = zip_open(archivePath.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &errorp);

			if(zipper)
			{
				bool success = addFilesRecursively(dirPath, dirPath, zipper);
				zip_close(zipper);
				return success;
			}
		}
	}

	return false;
}


#define ZIP_BUFFER_UNARCHIVE_SZIE 1024

bool unzipArchive(const std::string& archivePath, const std::string& dirPath)
{
	//MJLog("unzipArchive:%s->%s", archivePath, dirPath);
	static char buffer[ZIP_BUFFER_UNARCHIVE_SZIE];

	if(fileExistsAtPath(archivePath))
	{
		int errorp;
		zip_t *zipper = zip_open(archivePath.c_str(), ZIP_RDONLY, &errorp);

		if(zipper)
		{
			int numEntries = zip_get_num_entries(zipper, 0);

			struct zip_stat stat;
			for(int i = 0; i < numEntries; i++)
			{
				if (zip_stat_index(zipper, i, 0, &stat) == 0)
				{
					MJLog("found in zip file:%s", stat.name);
					size_t nameLength = strlen(stat.name);
					if(nameLength > 0)
					{
						std::string outputPath = pathByAppendingPathComponent(dirPath, stat.name);

						if(stat.name[nameLength - 1] == '/')
						{
							createDirectoriesIfNeededForDirPath(outputPath);
						}
						else
						{
							createDirectoriesIfNeededForFilePath(outputPath);

							struct zip_file *zipFile = zip_fopen_index(zipper, i, 0);
							if(!zipFile)
							{
								MJLog("ERROR:Failed to extract file:%s from zip archive:%s", stat.name, archivePath.c_str());
								zip_close(zipper);
								return false;
							}

							std::ofstream ofs(convertUtf8ToWide(outputPath), std::ios::binary | std::ios::out | std::ios::trunc);

							if(!ofs.is_open())
							{
								MJLog("Failed to unzip file, file save location not found:%s", outputPath);
								zip_fclose(zipFile);
								zip_close(zipper);
								return false;
							}

							int sum = 0;
							while (sum != stat.size) {
								size_t foundLength = zip_fread(zipFile, buffer, ZIP_BUFFER_UNARCHIVE_SZIE);
								if (foundLength < 0) 
								{
									MJLog("Failed to unzip file, file save location not found:%s", outputPath);
									ofs.close();
									zip_fclose(zipFile);
									zip_close(zipper);
								}

								ofs.write((const char*)buffer, foundLength);

								sum += foundLength;
							}

							zip_fclose(zipFile);
							ofs.close();
						}
					}
				}
			}

			zip_close(zipper);
			return true;
		}
	}


	return false;
}*/


void openFile(std::string filePath)
{
#ifdef WIN32
	ShellExecuteW(NULL, L"open", convertUtf8ToWide(filePath).c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif __APPLE__
    if(filePath.substr(0,4) == "http")
    {
        std::string cmd ="open \"" + filePath + "\"";
        system(cmd.c_str());
    }
    else
    {
        std::string revealCommand = "/usr/bin/osascript -e 'tell application \"Finder\" to reveal POSIX file \"" + filePath + "\"'";
        system(revealCommand.c_str());
        system("/usr/bin/osascript -e 'tell application \"Finder\" to activate'");
    }
#else
	MJLog("ERROR:openFile not implemented on this platform");
#endif
}
