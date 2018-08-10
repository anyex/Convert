
#include <cstdio>
#include <sstream>
#include <memory>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
#include <thread>
#include <set>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

int processNum = 0;
char swav[128] = { 0 };
/*自定义string类*/
class MyString {
public:
	MyString(char* s) :str(s) {  };
	MyString() :str() {};
	MyString(std::string&A) :str(A) {};

	void  DeleteMark(const char *mark) {

		str.erase(std::remove(str.begin(), str.end(), *mark), str.end());

	}

	void  Split(std::string &s1, std::string &s2, const std::string &flag)
	{
		int pos = str.find(flag);
		if (pos != -1)
		{
			s2 = str.substr(pos + 1, str.size() - 1);
			s1 = str.substr(0, pos);
		}
	}

	void Split(std::set<std::string>& s, const std::string&c) {
		std::string::size_type pos1, pos2;
		pos2 = str.find(c);
		pos1 = 0;
		while (std::string::npos != pos2)
		{
			std::string t = str.substr(pos1, pos2 - pos1);
			if (t.find("-"))
			{
				break;
			}
			s.insert(t);
			pos1 = pos2 + c.size();
			pos2 = str.find(c, pos1);

		}
		if (pos1 != str.length())
			s.insert(str.substr(pos1));

	}

	friend std::ostream& operator<<(std::ostream&, MyString&);
	int find(const std::string &s)
	{
		return str.find(s);
	}

	std::string str;

};
/*文件系列处理*/
class File {
public:
	/*传入输入路径和输出路径*/
	File(std::string &Path, std::string &output) :path(Path), out(output) {
		readFile();
	};


	/*重命名函数*/
	static void Rename(char *oldname, char *flag)
	{
		char newName[128], name[128];
		sscanf(oldname, "%[^.]", name);
		sprintf(newName, "%s%s%s", name, ".wav", flag);
		printf("%s", newName);
		rename(oldname, newName);
	}

	/*分割文件路径和文件名(将文件路径，文件名，扩展名拆开)*/
	void splitpath(const char *path, char*dir, char* fname, char*ext)
	{
		if (path == NULL)
		{
			dir[0] = '\0';
			fname[0] = '\0';
			return;
		}

		if (path[strlen(path)] == '/')
		{
			strcpy(dir, path);
			fname[0] = '\0';
			return;
		}
		char *whole_name = (char *)rindex(path, '/');

		printf("whole_name:  %s\n", whole_name + 1);

		

		snprintf(dir, whole_name-path+1, "%s", path);



		if (whole_name != NULL)
		{
			char *pext = rindex(whole_name, '.');

			if (pext != NULL)
			{
				strcpy(ext, pext + 1);
				int ret = whole_name - pext;
				snprintf(fname, pext - whole_name, "%s", whole_name + 1);
			}
			else
			{
				ext[0] = '\0';
				strcpy(fname, whole_name);
			}
		}

	}

	/*遍历文件*/
	void readFile()
	{
	
		DIR *input = opendir(path.c_str());
		std::string file;
		struct dirent *direntp;
		if (input)
		{
			while ((direntp = readdir(input)) != NULL)
			{
				if (direntp->d_type == 8)//文件
				{
					std::string filename(direntp->d_name);
					if (filename.find(".wav-") == -1)
					{
						file.append(path);
						file.append("/");
						file.append(direntp->d_name);

						/*if (access(file.c_str(), R_OK))//判断文件是否可读
						{
						set.insert(file);

						}*/
						set.insert(file);

						file.clear();
						continue;

						//if (set.size() >= 20)
						//sleep(5);
					}
					else
						continue;

				}
				if (direntp->d_type == 4)//目录
				{
					std::string s = direntp->d_name;

					if (s.find(".") == -1)
					{
						std::string Path = path;
						path.append("/");
						path.append(direntp->d_name);
						struct stat buf = {0};
						stat(path.c_str(), &buf);
						time_t sec_ts = buf.st_ctim.tv_sec;
						struct tm t= *gmtime(&sec_ts);

					
						struct tm local = getNowTm();

						if ((local.tm_year == t.tm_year)&&(local.tm_mon == t.tm_mon) &&( local.tm_mday == t.tm_mday))
						{
							readFile();
						
						}
						path = Path;

						continue;

					
					
					}
					else
						continue;
				}
			}
		}
	}

	/*产生一个待转换的wav文件路径和转换后的输出文件路径*/
	bool get()
	{
		if (!set.empty())
		{
			memset(wav, 0, sizeof(wav));
			memset(mp3, 0, sizeof(mp3));

			std::string file = *set.begin();

			int pos = file.find(path);
			int len = path.size();
			file.erase(pos, len);

			int b = file.find_last_of(".");

			file.erase(b);
		
			sprintf(mp3, "%s%s%s", out.c_str(), file.c_str(), ".mp3");

			char dir[128], name[128], ext[8];

			splitpath(mp3, dir, name, ext);

			if (access(dir, F_OK) == -1)
			{
				umask(0);
				CreateDir(dir);
			}
			set.erase(set.begin());

			return true;
			
		}

		return false;
	}

	/*创建多级目录*/
	int CreateDir(const char* pathName)
	{
		char DirName[256];
		strcpy(DirName, pathName);
		int i, len = strlen(DirName);
		if (DirName[len - 1] != '/')
		{
			strcat(DirName, "/");
		}
		for (i = 1; i <= len; i++)
		{
			if (DirName[i] == '/')
			{
				DirName[i] = 0;
				if (access(DirName, F_OK) != 0)
				{
					if (mkdir(DirName, 0755) == -1)
					{
						perror("mkdir error");
						return -1;
					}
				}
				DirName[i] = '/';
			}
		}
		return 0;
	}


	/*清空目录下的文件*/
	void DestroyDir(const char* pathName)
	{
		DIR *path = opendir(pathName);
		
		struct dirent *direntp;
		if (path)
		{
			while ((direntp = readdir(path)) != NULL)
			{
				char a[128];
				strcpy(a, pathName);
				strcat(a, "/");
				strcat(a, direntp->d_name);
				if (direntp->d_type == 4)
				{
					if ((!strcmp(direntp->d_name, ".")) || (!strcmp(direntp->d_name, "..")))
					{
						continue;
					}
					
					DestroyDir(a);
					rmdir(a);
				}

				if (direntp->d_type == 8)
				{
					unlink(a);
				}
			}
		}

	}

	/*定时程序*/
	static void Timing()
	{
		
	}

	/*执行转换*/
	void DoTrans() {
		processNum++;
		char *cmd[] = { "--preset fast standard",wav,mp3,NULL };
		if (execv("/usr/bin/lame", cmd) <0)
		{
			perror("error on exec");
			exit(0);
		}
	}

	 /*获取系统当前时间*/
	 time_t  getNowTime()
	{
		time_t tim;
		tim = time(NULL);
		
		return tim;
	}

	 /*获取系统当前时间*/
	 tm getNowTm()
	 {
		 tm ret;

		 time_t tim = time(NULL);

		 ret = *localtime(&tim);

		 return ret;
	 }

	 /*设定一个时间*/
	 time_t setTime(char* tim,char* format)
	 {
		 /*2018.07.30 01:00:00		时间的格式*/
		 
		 tm t = {0};
		 strptime(tim, format, &t);
		 time_t ret = mktime(&t);

		 return ret;
	 }

	 /*判断集合列表是否为空*/
	bool IsEmptySet()
	{
		return set.empty();
	}



	char wav[128];//待转换的wav文件(路径+文件名)
private:
	char mp3[128];//转换后的mp3文件(路径+文件名)

	char name[128];//文件名称
	std::set<std::string> set;//读取的文件列表
	std::string & path;//输入文件路径
	std::string & out; //输出文件路径
};

struct mytime {
	int year;
	int month;
	int day;
	int hours;
	int minute;
	int second;
};
/*回收子进程*/
void sig_handle(int num)
{
	int status;
	pid_t pid;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		if (WIFEXITED(status))
		{
			printf("child process exit pid[%6d],exit code[%d]\n", pid, WEXITSTATUS(status));
			processNum--;
			File::Rename(swav, "-");
		}
		else {
			printf("child process exit but...\n");

		}
	}
}

/*定时器函数*/
void sig_alrm(int signo)
{
	std::cout << "时间到！" << std::endl;

}

/*读取配置文件*/
class Ini {

public:
	Ini(const std::string& file) :filename(file) {
		getIni();
	};
	void getIni() {
		std::ifstream inflie(filename.data());
		if (inflie)
		{
			MyString s;
			std::string s1, s2, root;
			while (getline(inflie, s.str))
			{
				int pos = s.find("//");
				if (pos == 0)
				{
					continue;
				}
				s.DeleteMark(" ");
				s.DeleteMark("\r");
				s.DeleteMark("\t");

				if (s.find("[") != -1)
				{
					s.DeleteMark("[");
					s.DeleteMark("]");
					root = s.str;
					continue;
				}
				s.Split(s1, s2, "=");
				Inid[root][s1] = s2;
			}
		}
		else
		{
			std::cout << "can't open file!" << std::endl;
		}
		inflie.close();
	};
public:

	std::map <std::string, std::map<std::string, std::string> > Inid;
	const std::string& filename;
};



int main()
{

	Ini ini("/root/task/task.ini");
	File file(ini.Inid["set"]["input"], ini.Inid["set"]["output"]);

	file.get();
	file.DestroyDir("/root/tes");
	
	tm tm_;


	time_t t1 = file.setTime("2018.7.31 10:00:00", "%Y.%m.%d %H:%M:%S");

	

     signal(SIGALRM, sig_alrm);
	
	 alarm(difftime(t1, file.getNowTime()));

	

	 while (1) {};
	
	return 0;
}