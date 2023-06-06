#include <algorithm>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const int BlockSize = 1024;
const int BlockNum = 1024;
class Disk {
private:
  // 使用固定长度二维数组
  char **block;
  bool used[BlockNum];

public:
  Disk() {
    block = new char *[BlockNum];
    for (int i = 0; i < BlockNum; i++) {
      block[i] = new char[BlockSize];
      used[i] = false;
    }
  }
  // 写入数据
  bool write(string data, int index) {
    if (index < 0 || index >= BlockNum) {
      cout << "index out of range" << endl;
      return false;
    }
    if (data.size() > BlockSize) {
      cout << "data too large" << endl;
      return false;
    }
    // 判断是否已经使用
    if (used[index]) {
      cout << "block already used" << endl;
      return false;
    }
    for (int i = 0; i < data.size(); i++) {
      block[index][i] = data[i];
    }
    block[index][data.size()] = '\0'; // 字符串结尾
    used[index] = true;
    return true;
  }
  // 读取数据
  string read(int index) {
    if (index < 0 || index >= BlockNum) {
      cout << "index out of range" << endl;
      return "";
    }
    if (!used[index]) {
      cout << "block not used" << endl;
      return "";
    }
    string data = "";
    for (int i = 0; i < BlockSize; i++) {
      if (block[index][i] == '\0') {
        break;
      }
      data += block[index][i];
    }
    return data;
  }
  // 删除数据
  bool clear(int index) {
    if (index < 0 || index >= BlockNum) {
      cout << "index out of range" << endl;
      return false;
    }
    if (!used[index]) {
      cout << "block not used" << endl;
      return false;
    }
    used[index] = false;
    return true;
  }
  // 获取一个可用空间的下标
  int getFreeBlock() {
    for (int i = 0; i < BlockNum; i++) {
      if (!used[i]) {
        return i;
      }
    }
    return -1;
  }
  // 获取可用空间的总数
  int getFreeBlockNum() {
    int num = 0;
    for (int i = 0; i < BlockNum; i++) {
      if (!used[i]) {
        num++;
      }
    }
    return num;
  }
};
// 文件的属性
struct Attribute {
  string name;       // 文件名
  time_t createTime; // 创建时间
  time_t modifyTime; // 修改时间
  int size;          // 文件大小
};
// 包含文件属性和索引表，同时需要指向磁盘
class FCB {
private:
  vector<int> indexTable;
  Disk *disk;
  Attribute attribute;
  friend class Directory;

public:
  FCB(Disk *disk, string name) {
    this->disk = disk;
    this->attribute.name = name;
    this->attribute.createTime = time(0);
    this->attribute.modifyTime = this->attribute.createTime;
    this->attribute.size = 0;
  }
  // 文件的操作
  bool write(string data) {
    // 首先判断文件需要多少块，然后判断磁盘是否有足够的空间，然后写入数据
    int blockNum = data.size() / BlockSize + 1;
    if (disk->getFreeBlockNum() < blockNum) {
      cout << "not enough space" << endl;
      return false;
    }
    // 更新文件大小和修改时间
    attribute.size = data.size();
    attribute.modifyTime = time(0);

    for (int i = 0; i < blockNum; i++) {
      int index = disk->getFreeBlock();
      disk->write(data.substr(i * BlockSize, BlockSize), index);
      indexTable.push_back(index);
    }
    return true;
  }
  string read() {
    string data = "";
    for (int i = 0; i < indexTable.size(); i++) {
      data += disk->read(indexTable[i]);
    }
    return data;
  }
  // 删除文件
  bool clear() {
    for (int i = 0; i < indexTable.size(); i++) {
      disk->clear(indexTable[i]);
    }
    indexTable.clear();
    attribute.size = 0;
    attribute.modifyTime = time(0);
    return true;
  };
  // 获取文件名
  string getName() { return attribute.name; };
};

// 目录
class Directory {
private:
  vector<FCB> files;
  string name;
  time_t createTime;
  time_t modifyTime;
  vector<Directory> subDirectories;
  // 父目录
  Directory *parent;
  // 磁盘指针
  Disk *disk;
  friend class FileSystem;

public:
  Directory(string name, Directory *parent = NULL) {
    this->name = name;
    this->createTime = time(0);
    this->modifyTime = this->createTime;
    this->parent = parent;
    this->disk = disk;
  }
  string getName() { return name; };
  bool addFile(FCB file) {
    // 判断是否有重名的文件
    for (int i = 0; i < files.size(); i++) {
      if (files[i].getName() == file.getName()) {
        cout << "file already exists" << endl;
        return false;
      }
    }
    files.push_back(file);
    return true;
  }
  bool deleteFile(FCB file) {
    for (int i = 0; i < files.size(); i++) {
      if (files[i].getName() == file.getName()) {
        files.erase(files.begin() + i);
        return true;
      }
    }
    cout << "file not exists" << endl;
    return false;
  }
  bool addSubDirectory(string name) {
    // 判断是否有重名的目录
    for (int i = 0; i < subDirectories.size(); i++) {
      if (subDirectories[i].getName() == name) {
        cout << "directory already exists" << endl;
        return false;
      }
    }
    Directory directory = Directory(name, this);
    subDirectories.push_back(directory);
    return true;
  }
  bool deleteSubDirectory(string name) {
    for (int i = 0; i < subDirectories.size(); i++) {
      if (subDirectories[i].getName() == name) {
        subDirectories.erase(subDirectories.begin() + i);
        return true;
      }
    }
    cout << "directory not exists" << endl;
    return false;
  }
};

class FileSystem {
private:
  Disk disk;
  Directory root; // 根目录
  Directory *currentDirectory = &root;
  // 系统拥有FCB集合
  vector<FCB> FCBSet;

public:
  // ls命令
  FileSystem() : root("/"){};
  void ls() {
    for (int i = 0; i < currentDirectory->files.size(); i++) {
      cout << currentDirectory->files[i].getName() << endl;
    }
    for (int i = 0; i < currentDirectory->subDirectories.size(); i++) {
      cout << currentDirectory->subDirectories[i].getName() << endl;
    }
  }
  // cd命令
  void cd(string name) {
    for (int i = 0; i < currentDirectory->subDirectories.size(); i++) {
      if (currentDirectory->subDirectories[i].getName() == name) {
        currentDirectory = &currentDirectory->subDirectories[i];
        return;
      }
    }
    cout << "directory not exists" << endl;
  }
  // pwd命令
  void pwd() {
    string path = "";
    Directory *directory = currentDirectory;
    while (directory != NULL) {
      path = "/" + directory->getName() + path;
      directory = directory->parent;
    }
    cout << path << endl;
  }
  // mkdir命令
  void mkdir(string name) { currentDirectory->addSubDirectory(name); }
  // rmdir命令
  void rmdir(string name) { currentDirectory->deleteSubDirectory(name); }
  // touch命令
  void touch(string name) {
    FCB fcb = FCB(&disk, name);
    currentDirectory->addFile(fcb);
    FCBSet.push_back(fcb);
  }
  // echo命令
  void echo(string name, string data) {
    for (int i = 0; i < currentDirectory->files.size(); i++) {
      if (currentDirectory->files[i].getName() == name) {
        currentDirectory->files[i].write(data);
        return;
      }
    }
    // 文件不存在则创建
    touch(name);
    echo(name, data);
  }
  // 将内容写入文件
  void write(string name, string data) {
    for (int i = 0; i < currentDirectory->files.size(); i++) {
      if (currentDirectory->files[i].getName() == name) {
        currentDirectory->files[i].write(data);
        return;
      }
    }
    cout << "file not exists" << endl;
  }
  // cat命令
  void cat(string name) {
    for (int i = 0; i < currentDirectory->files.size(); i++) {
      if (currentDirectory->files[i].getName() == name) {
        cout << currentDirectory->files[i].read() << endl;
        return;
      }
    }
    cout << "file not exists" << endl;
  }
  // 返回上一级
  void back() {
    if (currentDirectory->parent != NULL)
      currentDirectory = currentDirectory->parent;
  }
};

int main() {
  // 通过循环读取命令
  FileSystem fileSystem;
  // 输入命令
  string command;
  // 帮我完善以下代码
  while (1) {
    cout << "command:";
    cin >> command;
    if (command == "ls")
      fileSystem.ls();
    else if (command == "back")
      fileSystem.back();
    else if (command == "cd") {
      cout << "Input dirName:";
      string name;
      cin >> name;
      fileSystem.cd(name);
    } else if (command == "pwd")
      fileSystem.pwd();
    else if (command == "mkdir") {
      cout << "Input dirName:";
      string name;
      cin >> name;
      fileSystem.mkdir(name);
    } else if (command == "rmdir") {
      cout << "Input dirName:";
      string name;
      cin >> name;
      fileSystem.rmdir(name);
    } else if (command == "touch") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      fileSystem.touch(name);
    } else if (command == "echo") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      cout << "Input content:";
      string data;
      cin >> data;
      fileSystem.echo(name, data);
    } else if (command == "write") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      cout << "Input content:";
      string data;
      cin >> data;
      fileSystem.write(name, data);
    } else if (command == "cat") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      fileSystem.cat(name);
    } else
      cout << "command not found" << endl;
  }
}