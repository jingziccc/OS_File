#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
const int BlockNum = 100;  // 块数量
const int BlockSize = 256; // 块大小
using namespace std;

struct Attribute {
  string name;       // 文件名
  time_t createTime; // 创建时间
  time_t modifyTime; // 修改时间
  int size;          // 文件大小
};

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
  // 使用析构函数释放内存
  ~Disk() {
    for (int i = 0; i < BlockNum; i++) {
      delete[] block[i];
    }
    delete[] block;
  }

  // 写入数据
  bool write(string data, int index) {
    if (index < 0 || index >= BlockNum) {
      cout << "index out of range" << endl;
      return false;
    }
    if (data.size() >= BlockSize) { // 修改此处判断条件
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
    // 在块的最后写入空字符 '\0'
    block[index][data.size()] = '\0';
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

  bool load(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
      std::cout << "Failed to open file: " << filename << std::endl;
      return false;
    }
    // 读取 used 数组
    file.read(reinterpret_cast<char *>(used), BlockNum);
    // 读取 block 数组
    for (int i = 0; i < BlockNum; i++) {
      if (used[i]) {
        file.read(block[i], BlockSize);
      }
    }
    file.close();
    return true;
  }

  bool store(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
      std::cout << "Failed to open file: " << filename << std::endl;
      return false;
    }
    // 写入 used 数组
    file.write(reinterpret_cast<char *>(used), BlockNum);
    // 写入 block 数组
    for (int i = 0; i < BlockNum; i++) {
      if (used[i]) {
        file.write(block[i], BlockSize);
      }
    }
    file.close();
    return true;
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
    for (int i = 0; i < BlockNum; i++)
      if (!used[i])
        return i;
    return -1;
  }
  // 获取可用空间的总数
  int getFreeBlockNum() {
    int num = 0;
    for (int i = 0; i < BlockNum; i++)
      if (!used[i])
        num++;
    return num;
  }

  void print() {
    cout << "num" << getFreeBlockNum() << endl;
    cout << "0is used" << used[0] << endl;
  }
};

class FCB {
private:
  vector<int> indexTable;
  Attribute attribute;
  friend class Directory;

public:
  FCB(string name = "") {
    this->attribute.name = name;
    this->attribute.createTime = time(0);
    this->attribute.modifyTime = this->attribute.createTime;
    this->attribute.size = 0;
  }

  // 保存 FCB 到文件
  bool store(std::ofstream &file) {
    if (!file.is_open()) {
      std::cout << "failed to open file" << std::endl;
      return false;
    }
    // 保存属性
    int nameLength = attribute.name.length();
    file.write((char *)&nameLength, sizeof(int));
    file.write(attribute.name.c_str(), nameLength);
    file.write((char *)&attribute.createTime, sizeof(time_t));
    file.write((char *)&attribute.modifyTime, sizeof(time_t));
    file.write((char *)&attribute.size, sizeof(int));
    // 保存索引表
    int size = indexTable.size();
    file.write((char *)&size, sizeof(int));
    for (int i = 0; i < size; i++) {
      file.write((char *)&indexTable[i], sizeof(int));
    }
    return true;
  }

  // 从文件加载 FCB
  bool load(std::ifstream &file) {
    if (!file.is_open()) {
      std::cout << "failed to open file" << std::endl;
      return false;
    }
    // 加载属性
    int nameLength = 0;
    file.read((char *)&nameLength, sizeof(int));
    char tempName[256];
    file.read(tempName, nameLength);
    tempName[nameLength] = '\0';
    attribute.name = std::string(tempName);
    file.read((char *)&attribute.createTime, sizeof(time_t));
    file.read((char *)&attribute.modifyTime, sizeof(time_t));
    file.read((char *)&attribute.size, sizeof(int));
    // 加载索引表
    int size = 0;
    file.read((char *)&size, sizeof(int));
    indexTable.resize(size);
    for (int i = 0; i < size; i++) {
      file.read((char *)&indexTable[i], sizeof(int));
    }
    return true;
  }

  void set(string name, int size) {
    this->attribute.name = name;
    this->attribute.modifyTime = time(0);
    this->attribute.size = size;
  }

  void print() {
    std::cout << "File name: " << this->attribute.name << std::endl;
    std::cout << "Create time: " << this->attribute.createTime << std::endl;
    std::cout << "Modify time: " << this->attribute.modifyTime << std::endl;
    std::cout << "File size: " << this->attribute.size << std::endl;
  }

  // 文件的操作
  bool write(string data, Disk *disk) {
    // 首先判断文件需要多少块，然后判断磁盘是否有足够的空间，然后写入数据
    int blockNum = data.size() / BlockSize + 1;
    if (disk->getFreeBlockNum() < blockNum) {
      cout << "not enough space" << endl;
      return false;
    }
    // 先清空原来的数据
    clear(disk);
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

  string read(Disk *disk) {
    string data = "";
    for (int i = 0; i < indexTable.size(); i++) {
      data += disk->read(indexTable[i]);
    }
    return data;
  }

  // 清除内容
  bool clear(Disk *disk) {
    for (int i = 0; i < indexTable.size(); i++)
      disk->clear(indexTable[i]);

    indexTable.clear();
    attribute.size = 0;
    attribute.modifyTime = time(0);
    return true;
  };

  // 获取文件名
  string getName() { return attribute.name; };
};
class Directory {
private:
  vector<Directory> subDirectories;
  vector<FCB> files;
  Attribute attribute;
  // 父目录
  Directory *parent;
  friend class FileSystem;

public:
  Directory(string name = "", Directory *parent = NULL) {
    this->attribute.name = name;
    this->attribute.createTime = time(0);
    this->attribute.modifyTime = this->attribute.createTime;
    this->attribute.size = 0;
    this->parent = parent;
  }
  ~Directory() {
    // 析构函数
  }

  string getName() { return this->attribute.name; };

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

  void setParent(Directory *parent) { this->parent = parent; }
  void setName(string name) { attribute.name = name; }

  // 保存 Directory 到文件
  bool store(std::ofstream &file) {
    if (!file.is_open()) {
      std::cout << "failed to open file" << std::endl;
      return false;
    }
    // 保存属性
    int nameLength = attribute.name.length();
    file.write((char *)&nameLength, sizeof(int));
    file.write(attribute.name.c_str(), nameLength);
    file.write((char *)&attribute.createTime, sizeof(time_t));
    file.write((char *)&attribute.modifyTime, sizeof(time_t));
    file.write((char *)&attribute.size, sizeof(int));

    // 保存子目录
    int size = subDirectories.size();
    file.write((char *)&size, sizeof(int));
    for (int i = 0; i < size; i++) {
      subDirectories[i].store(file);
    }

    // 保存文件
    size = files.size();
    file.write((char *)&size, sizeof(int));
    for (int i = 0; i < size; i++) {
      files[i].store(file);
    }

    return true;
  }

  // 从文件加载 Directory
  bool load(std::ifstream &file) {
    if (!file.is_open()) {
      std::cout << "failed to open file" << std::endl;
      return false;
    }
    // 加载属性
    int nameLength = 0;
    file.read((char *)&nameLength, sizeof(int));
    char tempName[256];
    file.read(tempName, nameLength);
    tempName[nameLength] = '\0';
    attribute.name = std::string(tempName);
    file.read((char *)&attribute.createTime, sizeof(time_t));
    file.read((char *)&attribute.modifyTime, sizeof(time_t));
    file.read((char *)&attribute.size, sizeof(int));

    // 加载子目录
    int size = 0;
    file.read((char *)&size, sizeof(int));
    subDirectories.resize(size);
    for (int i = 0; i < size; i++) {
      subDirectories[i].load(file);
    }

    // 加载文件
    size = 0;
    file.read((char *)&size, sizeof(int));
    files.resize(size);
    for (int i = 0; i < size; i++) {
      files[i].load(file);
    }

    return true;
  }
};

class FileSystem {
private:
  Disk *disk;
  Directory root; // 根目录
  Directory *currentDirectory;

public:
  FileSystem() {
    disk = new Disk();
    root = Directory("/");
    currentDirectory = &root;
  }
  // ls命令
  FileSystem(Disk *_disk, Directory _root) {
    this->disk = _disk;
    this->root = _root;
    this->currentDirectory = &root;
  }

  void store() {
    disk->store("disk.bin");
    std::ofstream rootDir("directory.bin", std::ios::binary);
    root.store(rootDir);
  }
  // 传入的参数是Directory的引用，作用是为其子目录设置parent
  void setParent(Directory &directory) {
    for (int i = 0; i < directory.subDirectories.size(); i++) {
      directory.subDirectories[i].setParent(&directory);
      setParent(directory.subDirectories[i]);
    }
  }

  bool Init(Disk &disk) {

    // 从文件加载磁盘
    // 如果没有磁盘文件，则创建一个磁盘文件
    // 找有没有"disk.bin"和"directory.bin"文件, 如果没有就直接返回错误
    std::ifstream diskFile("disk.bin", std::ios::binary);
    std::ifstream rootDir("directory.bin", std::ios::binary);
    if (!diskFile.is_open() || !rootDir.is_open()) {
      std::cout << "failed to open file" << std::endl;
      cout << "create a new one" << endl;
      return false;
    }

    disk.load("disk.bin");

    this->disk = &disk;
    cout << "ok";
    this->disk->print();
    root.load(rootDir);
    // 根据根目录一步步设置所有目录的关系Directory* parent
    root.setParent(NULL);
    currentDirectory = &root;
    // 为所有子目录设置parent
    setParent(root);
    return true;
  }
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
    FCB fcb = FCB(name);
    currentDirectory->addFile(fcb);
  }
  // echo命令
  void echo(string name, string data) {
    for (int i = 0; i < currentDirectory->files.size(); i++) {
      if (currentDirectory->files[i].getName() == name) {
        currentDirectory->files[i].write(data, disk);
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
        currentDirectory->files[i].clear(disk);
        currentDirectory->files[i].write(data, disk);
        return;
      }
    }
    cout << "file not exists" << endl;
  }
  // cat命令
  void cat(string name) {
    for (int i = 0; i < currentDirectory->files.size(); i++) {
      if (currentDirectory->files[i].getName() == name) {
        cout << currentDirectory->files[i].read(disk) << endl;
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
  // 从文件中读取FileSystem对象的状态，如果文件不存在则创建一个新的FileSystem对象
  FileSystem fileSystem;
  Disk disk;
  fileSystem.Init(disk);
  // 输入命令
  string command;

  while (1) {
    cout << "command:";
    cin >> command;
    if (command == "ls")
      fileSystem.ls();
    else if (command == "back")
      fileSystem.back();
    else if (command == "cat") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      fileSystem.cat(name);
    } else if (command == "cd") {
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
    } else if (command == "echo") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      cout << "Input content:";
      string content;
      cin.ignore();
      getline(cin, content);
      fileSystem.echo(name, content);
    } else if (command == "touch") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      fileSystem.touch(name);
    } else if (command == "exit") {
      break;
    } else if (command == "write") {
      cout << "Input fileName:";
      string name;
      cin >> name;
      cout << "Input content:";
      string content;
      cin.ignore();
      getline(cin, content);
      fileSystem.write(name, content);
    } else
      cout << "command not found" << endl;
  }
  // 存储FileSystem对象的状态
  fileSystem.store();
  return 0;
}