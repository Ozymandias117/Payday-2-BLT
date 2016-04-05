//WINDOWS ONLY FILE!
//Some other stuff might be aswell, but this is rather desperately windows only.

#include "util.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#include <fstream>
#include <streambuf>

using namespace std;

namespace Util{
	vector<string> GetDirectoryContents(std::string path, bool dirs){
		vector<string> files;

        DIR* dir = opendir( path.c_str() );
        Logging::Log( path.c_str(), Logging::LOGGING_LOG );

        if( NULL != dir )
        {
            struct dirent* next = readdir( dir );
            while( NULL != next )
            {
                bool isDir = ( DT_DIR == next->d_type );

                if( ( dirs && isDir ) || ( !dirs && !isDir ) )
                {
                    files.push_back( next->d_name );
                }
                next = readdir( dir );
            }
        }
        else
        {
            // From Windows version
           throw - 1;
        }

		return files;
	}

	string GetFileContents(string filename){
		ifstream t(filename);
		string str;

		t.seekg(0, std::ios::end);
		str.reserve(t.tellg());
		t.seekg(0, std::ios::beg);
		str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
		
		return str;
	}

	bool DirectoryExists(std::string dir){
        struct stat file_info;
        bool ret = false;

        if( 0 == stat( dir.c_str(), &file_info ) )
        {
            ret = S_ISDIR( file_info.st_mode );
        }

		return ret;
	}

	void EnsurePathWritable(std::string path){
		int finalSlash = path.find_last_of('/');
		std::string finalPath = path.substr(0, finalSlash);
		if (DirectoryExists(finalPath)) return;
		CreateDirectoryPath(finalPath.c_str());
	}

	bool RemoveEmptyDirectory(std::string dir){
		return remove( dir.c_str() );
	}

	bool CreateDirectoryPath(std::string path){
        mode_t mode = 0755;
		std::string newPath = "";
		std::vector<std::string> paths = Util::SplitString(path.c_str(), '/');
		for (auto i : paths) {
			newPath = newPath + i + "/";
			mkdir( newPath.c_str(), mode );
		}
		return true;
	}

	std::vector<std::string> &SplitString(const std::string &s, char delim, std::vector<std::string> &elems) {
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			if (!item.empty()){
				elems.push_back(item);
			}
		}
		return elems;
	}

	std::vector<std::string> SplitString(const std::string &s, char delim) {
		std::vector<std::string> elems;
		SplitString(s, delim, elems);
		return elems;
	}

}
