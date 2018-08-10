#include <cstdio>
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
#include <sys/sem.h>
#include <dirent.h>
#include <string.h>

#define DBG(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)  
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

	//分割文件路径和文件名(将文件路径，文件名，扩展名拆开)
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

		snprintf(dir, whole_name - path + 1, "%s", path);



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
		struct stat buf;
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
					if (filename.find(".wavbak") == -1&&(filename.find(".mp3") == -1))
					{
						file.append(path);
						file.append("/");
						file.append(direntp->d_name);

						//判断文件是否被占用
						if (Lsof(file.c_str()) == 0)
						{
							set.insert(file);
							file.clear();
						}

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
						stat(path.c_str(), &buf);
						struct stat buf = { 0 };
						//获取目录状态
						stat(path.c_str(), &buf);
						//获取目录的创建时间
						time_t sec_ts = buf.st_ctim.tv_sec;
						
						struct tm t = *gmtime(&sec_ts);

						struct tm local = getNowTm();
						//是否是当天创建
						if ((local.tm_year == t.tm_year) && (local.tm_mon == t.tm_mon) && (local.tm_mday == t.tm_mday))
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

	/*创建目录*/
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

	/*产生一个待转换的wav文件路径和转换后的输出文件路径*/
	bool get()
	{
		if (!set.empty())
		{
			memset(wav, 0, sizeof(wav));
			memset(mp3, 0, sizeof(mp3));
			memset(localMp3, 0, sizeof(localMp3));
			memset(netMp3, 0, sizeof(netMp3));

			std::string file = *set.begin();
			strcpy(wav, file.c_str());
			int b = file.find_last_of(".");
			file.erase(b);
			file.append(".mp3");
			strcpy(localMp3, file.c_str());

			file.find(path);
			file.erase(file.find(path), path.length());

			sprintf(netMp3, "%s%s", out.c_str(), file.c_str());

			std::string curMp3(netMp3);
			curMp3.erase(0, curMp3.find(":")+1);
			curMp3.erase(curMp3.find_last_of("/"));
			strcpy(mp3, curMp3.c_str());

			//	sprintf(mp3, "%s%s%s", out.c_str(), file.c_str(), ".mp3");
			char dir[128], name[128], ext[8];
			splitpath(localMp3, dir, name, ext);
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

	/*执行转换,转存*/
	void Covert() {
		processNum++;
		char *cmd[] = { "sh","/root/test.sh",wav,localMp3,localMp3,netMp3,mp3,(char*)0 };
		if (execv("/bin/sh", cmd) < 0)
		{
			perror("error on exec");
			exit(0);
		}
	}
	/*判断文件是否正在使用*/
	int Lsof(const char *strFileSubPath)  //其中strFileSubPath包括文件名
	{
		DIR     *dir, *fdir;
		struct  dirent *dirent, *fdirent;
		char    strTmpPath[256] = "\0";
		char    strSubPath[256] = "\0";
		char    strHold[256] = "\0";

		if (NULL == strFileSubPath)
		{
			DBG(0, "Lsof函数参数为空");
			return -1;
		}

		dir = opendir("/proc");
		if (NULL == dir)
		{
			DBG(0, "打开文件路径[%s]出错，错误信息[%s]", "/proc", strerror(errno));
			return -1;
		}
		while (NULL != (dirent = readdir(dir)))
		{
			if (0 == strcmp(".", dirent->d_name) || 0 == strcmp("..", dirent->d_name))
				continue;

			sprintf(strTmpPath, "/proc/%s/fd/", dirent->d_name);

			fdir = opendir(strTmpPath);
			if (NULL == fdir)
			{
				continue;
			}
			while (NULL != (fdirent = readdir(fdir)))
			{
				if (0 == strcmp(".", fdirent->d_name) || 0 == strcmp("..", fdirent->d_name))
					continue;

				sprintf(strSubPath, "%s/%s", strTmpPath, fdirent->d_name);

				memset(strHold, 0, 256);
				readlink(strSubPath, strHold, 256);

				if (0 == strcmp(strFileSubPath, strHold))
				{
					return 1;
				}
			}
			closedir(fdir);
		}
		closedir(dir);

		return 0;
	}

	/*判断集合列表是否为空*/
	bool IsEmptySet()
	{
		return set.empty();
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

	char wav[128];//待转换的wav文件(路径+文件名)
private:
	char mp3[128];//转存后的mp3文件(复制路径+文件名)
	char netMp3[128];//转存的mp3文件路径(带网络地址)
	char localMp3[128];//本地mp3路径
	char name[128];//文件名称
	
	std::set<std::string> set;//读取的文件列表
	std::string & path;//输入文件路径
	std::string & out; //转存文件路径
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
			File::Rename(swav, "bak");
		}
		else {
			printf("child process exit but...\n");

		}
	}
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


	Ini ini("/root/task.ini");
	File file(ini.Inid["set"]["audiodir"], ini.Inid["set"]["transdir"]);

	signal(SIGCHLD, sig_handle);//为子进程退出注册事件
	
	while (true)
	{
	if (processNum < 2&&(!file.IsEmptySet()))
	{
	if (vfork() == 0)
	{
		if (!file.get())
			exit(0);
			strcpy(swav, file.wav);
			file.Covert();
	}
	else
	{
		continue;
	}
	}
	else if(processNum<2)
	{
		file.readFile();
	if (file.IsEmptySet())
		std::cout << "wav be finished!" << std::endl;
	}
	else {
		sleep(1);
		}
	}
	return 0;
}

