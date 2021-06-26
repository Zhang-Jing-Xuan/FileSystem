#include "FileSys.h"

FileSys::FileSys()
{
    prefix = "Administrator:";
    ifstream checkFile(MAINFILE);
    if (!checkFile.good())
    {
        checkFile.close();
        boot(); //启动
    }
    else
        checkFile.close();
    memset(h, -1, sizeof(h));        //初始化邻接表表头为空
    id = 0;                          // 文件夹的个数
    current = 0;                     // 当前处于哪一级目录
    total = 0;                       // 文件和文件夹的总数
    memset(depth, 0, sizeof(depth)); //初始化深度为空
}

void FileSys::run()
{
    printf("请输入regist以注册用户，输入login以登录账号，按Ctrl+C退出\n");
    p(prefix);
    string command = "";
    while (cin >> command)
    {
        transform(command.begin(), command.end(), command.begin(), ::tolower);
        if (command == "login" && login())
            break;
        if (command == "regist")
            adduser();
        if (command != "login" && command != "regist")
        {
            printf("请输入regist以注册用户，输入login以登录账号，按Ctrl+C退出\n");
        }
        p(prefix);
    }
    init();
    for (;;)
    {
        p(prefix);
        cin >> command;
        transform(command.begin(), command.end(), command.begin(), ::tolower);
        if (command == "dir")
            printDir();
        else if (command == "create")
            createFile();
        else if (command == "delete")
            deleteFile();
        else if (command == "open")
            openFile();
        else if (command == "read")
            readFile();
        else if (command == "overwrite")
            writeFile();
        else if (command == "append")
            appendFile();
        else if (command == "copy")
            copyFile();
        else if (command == "close")
            closeFile();
        else if (command == "info")
            info();
        else if (command == "exit")
            break;
        else if (command == "mkdir")
            mkdir();
        else if (command == "cd")
        {
            string dst;
            cin >> dst;
            if (dst == "..")
            {
                cdback();
            }
            else
            {
                cddir(dst);
            }
        }
        else
        {
            printf("命令出错!\n");
        }
    }
}

void FileSys::printDir()
{
    printf("Folder:\n");
    for (int i = h[current]; ~i; i = ne[i])
    {
        int j = e[i];
        if (depth[j] == depth[current] + 1)
        {
            cout << hash[j] << "\t";
        }
    }
    printf("\n");
    if (fileEntrys.size() == 0)
        return;
    cout << left << setw(15) << "FileName" << setw(9) << "Address" << setw(15) << "Protect code" << setw(10) << "Length" << endl;
    for (FileEntry &i : fileEntrys)
    {
        if (depth[rhash[i.fileName]] == depth[current])
        {
            cout << left << setw(15) << i.fileName << setw(9) << i.address << setw(15) << pcToBinary(i.protectCode) << setw(10) << i.length << endl;
        }
    }
}

void FileSys::enterFileName()
{
    p("fileName:");
    cin >> fileName;
}

void FileSys::createFile()
{
    enterFileName(); //FileSys的filename
    if (query(fileName.c_str()) == 1)
    {
        printf("已存在同名文件！\n");
        return;
    }
    insert(fileName.c_str());
    pln("Protect Code(three-bit integer): ");
    int pc; //保护码
    cin >> pc;
    FileEntry newfe;
    auto len = fileName.size();
    len = len < 30 ? len : 29;
    fileName.copy(newfe.fileName, len); //把fileName拷贝到newfe.fileName
    newfe.fileName[len] = '\0';
    newfe.protectCode = pc;
    //新的inode
    Inode newinode(0);
    auto address = inodeTable.add(&newinode); //添加一个索引节点
    newfe.address = address;                  //newfe文件地址
    fileEntrys.push_back(newfe);
    file.setInode(&rootDir);                      // 绑定inode到根目录
    file.writeDir(fileEntrys, user.inodeAddress); // 写inode为目录
    file.setInode(&newinode);                     // 绑定inode到新建的节点
    file.writeSelf(address);                      //将本身i-node重新写入i-node表
    file.setInode(NULL);

    total++; // 文件+文件夹的数量+1
    file.dir = false; // 不是文件夹
    depth[total] = depth[current]; // 该文件的深度和该文件夹的深度相同
    hash[total] = fileName; // 更新哈希表和反哈希表
    rhash[fileName] = total;
}

int FileSys::openFile() // 打开文件
{
    enterFileName(); // 用户输入文件名
    if (query(fileName.c_str()) == 0) // 到字典树中查找，判断该文件名是否已经存在，如果不存在
    {
        pln("No Such File"); // 输出不存在此文件并换行
        return -1;
    }
    for (size_t i = 0; i < fileEntrys.size(); ++i) // 遍历所有文件
    {
        if (string(fileEntrys[i].fileName) == fileName) //找到文件
        {
            nowFileEntry = &fileEntrys[i]; // 得到fileEntry[i]的地址
            // 绑定inode到找到的文件
            file.setInode(inodeTable.getInode(fileEntrys[i].address));
            pln("Open Successfully"); //输出打开成功并换行
            return 1;
        }
    }
    return -1;
    // pln("No Such File");
    // return -1;
}

void FileSys::deleteFile()
{
    enterFileName();
    for (auto i = fileEntrys.begin(); i < fileEntrys.end(); ++i)
    {
        if (string(i->fileName) == fileName)
        {
            fileEntrys.erase(i);
            // 绑定inode到根目录
            file.setInode(&rootDir);
            file.writeDir(fileEntrys, user.inodeAddress);
            pln("Delete Successfully");
            return;
        }
    }
    pln("No Such File");
}

void FileSys::closeFile()
{
    if (file.getInode() == NULL)
    {
        pln("Close Failed");
        return;
    }
    // pln("delete success");
    delete file.getInode();
    file.setInode(NULL);
    nowFileEntry = NULL;
    pln("Close Successfully");
}

void FileSys::readFile()
{
    if (nowFileEntry == NULL || nowFileEntry->fileName != fileName)
    {
        printf("请先打开该文件！\n");
        return;
    }
    pln(file.readAll());
}

void FileSys::pln(const string &s) { cout << s << endl; }
void FileSys::p(const string &s) { cout << s; }

void FileSys::copyFile()
{
    int success = 1; // 标志源文件是否打开成功
    success = openFile(); 
    if (success == -1) // 如果打开失败
    {
        return;
    }
    copyRead(); // 读取源文件的内容
    success = openFile(); // 标志目标文件是否打开成功
    if (success == -1) // 如果打开失败
    {
        return;
    }
    copyWrite(); // 写入目标文件
    tmp = ""; // 清空缓存
}

void FileSys::copyRead()
{
    tmp = file.readAll(); // 读取源文件的内容
}

void FileSys::copyWrite() // 写入目标文件
{
    auto len = file.write(tmp, nowFileEntry->address); // 写到当前的i-node拥有的block里面
    nowFileEntry->length = len; // 更新文件长度
    file.setInode(&rootDir); // 绑定到根节点
    file.writeSelf(user.inodeAddress);            // 将本身i-node重新写入i-node表
    file.writeDir(fileEntrys, user.inodeAddress); //写inode为目录
}

void FileSys::appendFile() 
{
    if (nowFileEntry == NULL || nowFileEntry->fileName != fileName)
    {
        printf("请先打开该文件！\n");
        return;
    }
    copyRead();
    pln("write here. End with '$EOF'");
    string res = "", line = "";
    cin.clear();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //清除当前行
    for (;;)
    {
        getline(cin, line);
        auto pos = line.find("$EOF");
        if (pos != string::npos)
        {
            line[pos] = '\n';
            line[pos + 1] = '\0';
            res += line.substr(0, pos + 2);
            break;
        }
        line += '\n';
        res += line;
    }
    res = res.substr(0, res.size() - 1);
    tmp = tmp.substr(0, tmp.size() - 1);
    // cout<<tmp<<" "<<res<<endl;
    res = tmp + res;
    // res=res.substr(0,res.size()-1);
    // cout<<"res="<<res<<endl;
    auto len = file.write(res, nowFileEntry->address); // 写到当前的i-node拥有的block里面
    nowFileEntry->length = len;
    file.setInode(&rootDir);
    file.writeSelf(user.inodeAddress);            // 将本身i-node重新写入i-node表
    file.writeDir(fileEntrys, user.inodeAddress); //写inode为目录
    tmp = "";
}

void FileSys::writeFile()
{
    if (nowFileEntry == NULL || nowFileEntry->fileName != fileName)
    {
        printf("请先打开该文件！\n");
        return;
    }
    pln("write here. End with '$EOF'");
    string res = "", line = "";
    cin.clear();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //清除当前行
    for (;;)
    {
        getline(cin, line);
        auto pos = line.find("$EOF");
        if (pos != string::npos)
        {
            line[pos] = '\n';
            line[pos + 1] = '\0';
            res += line.substr(0, pos + 2);
            break;
        }
        line += '\n';
        res += line;
    }
    auto len = file.write(res, nowFileEntry->address); // 写到当前的i-node拥有的block里面
    nowFileEntry->length = len;
    file.setInode(&rootDir);
    file.writeSelf(user.inodeAddress);            // 将本身i-node重新写入i-node表
    file.writeDir(fileEntrys, user.inodeAddress); //写inode为目录
}

void FileSys::boot()
{
    ofstream sysfile(MAINFILE, ios::binary);                           //写入二进制文件man.dat
    size_t i = 0;                                                      // 文件指针置为文件开头
    sysfile.write(reinterpret_cast<const char *>(&i), sizeof(size_t)); //将从文件开头开始size_t大小的内容写入sysfile
    sysfile.close();                                                   // 关闭二进制文件

    sysfile.open(DATAFILE, ios::binary); //打开二进制文件data.dat
    bool tempTable[200] = {0}; // 全部为零的bool数组
    sysfile.seekp(0); // 文件指针指向开头
    sysfile.write(reinterpret_cast<const char *>(tempTable), INODE_BIT_SIZE); //初始化二进制文件data.dat
    Bitmap tempBitmap; // 实例化临时的位示图
    tempBitmap.init(); // 初始化临时的位示图
    sysfile.seekp(INODE_TABLE_SIZE); // 调整文件指针
    sysfile.write(reinterpret_cast<const char *>(&tempBitmap), BITMAP_SIZE); //写到临时的位示图中
    sysfile.close(); // 关闭二进制文件data.dat

    file.readBitmap(); //读取位示图
    inodeTable = InodeTable(); //实例化索引表
}

string FileSys::pcToBinary(int n)
{
    string res = "";
    auto convert = [&res](int i) // lambda表达式
    {
        switch (i)
        {
        case 0:
            res += "000";
            break;
        case 1:
            res += "001";
            break;
        case 2:
            res += "010";
            break;
        case 3:
            res += "011";
            break;
        case 4:
            res += "100";
            break;
        case 5:
            res += "101";
            break;
        case 6:
            res += "110";
            break;
        case 7:
            res += "111";
            break;
        }
    };
    convert(n / 100);
    convert(n % 100 / 10);
    convert(n % 10);
    return res;
}

bool FileSys::login()
{
    string username, password;
    p("Username: ");
    cin >> username;
    p("Password: ");
    cin >> password;
    //读取主文件的用户，进行匹配登录
    ifstream sysfile(MAINFILE, ios::in | ios::binary);
    size_t userNum = 0;
    sysfile.read(reinterpret_cast<char *>(&userNum), sizeof(size_t));
    for (size_t i = 0; i < userNum; ++i)
    {
        sysfile.seekg(sizeof(size_t) + i * sizeof(User));
        sysfile.read(reinterpret_cast<char *>(&user), sizeof(User));
        //匹配成功，直接退出
        if (user.username == username && user.password == password)
        {
            pln("Login successfully");
            sysfile.close();
            return true;
        }
    }
    sysfile.close();
    //匹配失败，继续登录
    pln("Login Failed");
    return false;
}

void FileSys::adduser()
{
    User newUser;
    p("Username:");
    cin >> newUser.username;
    
    unordered_map<string,bool> judgeUser;
    ifstream _sysfile(MAINFILE, ios::in | ios::binary);
    size_t userNum = 0;
    _sysfile.read(reinterpret_cast<char *>(&userNum), sizeof(size_t));
    for (size_t i = 0; i < userNum; ++i)
    {
        _sysfile.seekg(sizeof(size_t) + i * sizeof(User));
        _sysfile.read(reinterpret_cast<char *>(&user), sizeof(User));
        judgeUser[user.username]=true;
    }
    _sysfile.close();
    if(judgeUser[newUser.username]){
        cout<<"用户名已存在，请重新注册！"<<endl;
        return ;
    }
    
    p("Password:");
    cin >> newUser.password;
    Inode root(0);
    newUser.inodeAddress = inodeTable.add(&root);
    fstream sysfile(MAINFILE, ios::in | ios::out | ios::binary);
    userNum = 0;
    sysfile.read(reinterpret_cast<char *>(&userNum), sizeof(size_t));
    sysfile.seekp(sizeof(size_t) + userNum * sizeof(User)); //将文件写入位置移到sizeof(size_t) + userNum * sizeof(User)
    sysfile.write(reinterpret_cast<const char *>(&newUser), sizeof(User));
    ++userNum;
    sysfile.seekp(0); // 将文件写入位置移到开头
    sysfile.write(reinterpret_cast<const char *>(&userNum), sizeof(size_t));
    sysfile.close();
}

void FileSys::info()
{
    readnum();
    printf("----------磁盘容量：1M----------\n");
    printf("----------用户数量：%d------------\n", user_num);
    printf("            命令\n");
    printf("\
- info      列出文件系统信息\n\
- login     用户登录\n\
- dir       列出文件夹和文件目录\n\
- create    创建文件\n\
- delete    删除文件\n\
- open      打开文件\n\
- close     关闭文件\n\
- copy      拷贝文件\n\
- read      读文件\n\
- append    在文件末尾追加\n\
- overwrite 覆盖原文件\n\
- mkdir     新建目录\n\
- cd        进入目录\n\
");
}

void FileSys::readnum()
{
    fstream sysfile(MAINFILE, ios::in | ios::out | ios::binary);
    size_t userNum = 0;
    sysfile.read(reinterpret_cast<char *>(&userNum), sizeof(size_t));
    user_num = userNum;
}

void FileSys::init()
{
    prefix += "~ " + string(user.username) + "$ "; //~zjx$
    ifstream sysfile(DATAFILE, ios::in | ios::binary);
    sysfile.seekg(user.inodeAddress); //根目录的inode地址
    sysfile.read(reinterpret_cast<char *>(&rootDir), sizeof(Inode));
    sysfile.close();
    file.setInode(&rootDir);
    FileEntry temp;
    nowFileEntry = &temp;
    fileEntrys = file.getDir();
}

void FileSys::add(int a, int b)
{
    e[id] = b;
    ne[id] = h[a];
    h[a] = id++;
}

void FileSys::mkdir()
{
    string folderName;
    cin >> folderName;
    total++;
    depth[total] = depth[current] + 1;
    add(current, total);
    add(total, current);
    hash[total] = folderName;
    rhash[folderName] = total;
}

void FileSys::cddir(string dst)
{
    for (int i = h[current]; ~i; i = ne[i])
    {
        int j = e[i];
        if (hash[j] == dst)
        {
            current = rhash[dst];
            string x = prefix.substr(0, prefix.size() - 2);
            prefix = x + "/" + dst + "$ ";
            return;
        }
    }
    printf("目录不存在！\n");
}

void FileSys::cdback()
{
    if (current == 0)
    {
        printf("您已在根目录，无法回退！\n");
        return;
    }
    for (int i = h[current]; ~i; i = ne[i])
    {
        int j = e[i];
        if (depth[j] < depth[current])
        {
            current = j;
            for (int i = prefix.size(); i >= 0; i--)
            {
                if (prefix[i] == '/')
                {
                    prefix = prefix.substr(0, i);
                    prefix = prefix + "$ ";
                    return;
                }
            }
        }
    }
    printf("回退失败！\n");
    return;
}