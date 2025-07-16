#include <iostream>
// #include <vector>
#include <fstream>
#include <cstdint>
#include <unordered_map>
using namespace std;

#define CTRL_KEY(k) ((k) & 0x1f)

ofstream outStream;
string fileName; // fileName need to be global

struct editorEachChar
{
    pair<uint8_t, uint8_t> pos; // let first of pos is row,second is col
    char c;
};

struct editorEachChar EEC;

struct PairHash
{
    size_t operator()(const std::pair<uint8_t, uint8_t> &p) const
    {
        return (static_cast<size_t>(p.first) << 8) | p.second;
    }
};

unordered_map<std::pair<uint8_t, uint8_t>, editorEachChar, PairHash> editorCharUnorderMap;

enum editorKey
{
    ARROW_LEFT = 1,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};

void initEditor()
{
    //CKernel::Get()->setRaw(); // if enable,can't display right

    //CKernel::Get()->clear();

    EEC.pos.first = 1;
    EEC.pos.second = 1; // circle's screen position start with 1
}

char editorReadKey()
{
    int nread;

    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        // if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b')
    {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';

        if (seq[0] == '[')
        {
            switch (seq[1])
            {
            case 'A':
                return ARROW_UP;
            case 'B':
                return ARROW_DOWN;
            case 'C':
                return ARROW_RIGHT;
            case 'D':
                return ARROW_LEFT;
            }
        }

        return '\x1b';
    }
    else
    {
        return c;
    }
}

void editorMoveCursor(char key)
{
    switch (key)
    {
    case ARROW_LEFT:
        EEC.pos.first--;
        break;
    case ARROW_RIGHT:
        EEC.pos.first++;
        break;
    case ARROW_UP:
        EEC.pos.second--;
        break;
    case ARROW_DOWN:
        EEC.pos.second++;
        break;
    }
}

void writeMapToBinary(const unordered_map<pair<uint8_t, uint8_t>, editorEachChar, PairHash> &charMap,
                      const string &filename)
{
    ofstream outFile(filename, ios::binary);
    if (!outFile)
    {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }

    for (const auto &[pos, eec] : charMap)
    {

        outFile.write(reinterpret_cast<const char *>(&pos.first), sizeof(uint8_t));

        outFile.write(reinterpret_cast<const char *>(&pos.second), sizeof(uint8_t));

        outFile.write(&eec.c, sizeof(char));
    }

    outFile.close();
}

std::unordered_map<std::pair<uint8_t, uint8_t>, editorEachChar, PairHash> readFromBinaryFile(
    const std::string &filename)
{
    std::unordered_map<std::pair<uint8_t, uint8_t>, editorEachChar, PairHash> charMap;
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Failed to open file for reading." << std::endl;
        return charMap;
    }

    uint8_t cx, cy;
    char c;
    while (inFile.read(reinterpret_cast<char *>(&cx), sizeof(uint8_t)))
    {
        inFile.read(reinterpret_cast<char *>(&cy), sizeof(uint8_t));
        inFile.read(&c, sizeof(char));

        charMap[{cx, cy}] = {{cx, cy}, c};
    }

    printf("%d\n",charMap.size());

    return charMap;
}

void displayFileContent(string fileName)
{
    auto loadedMap = readFromBinaryFile(fileName);

    for (const auto &[pos, eec] : loadedMap)
    {
        string tmp = "\x1b[";
        tmp.append(to_string(pos.first));
        tmp.append(";");
        tmp.append(to_string(pos.second));
        tmp.append("H");
        char *a = (char *)tmp.c_str();

        printf("%s\n",a);

        //CKernel::Get()->printString(a);
        char d=eec.c;
        //CKernel::Get()->printChar(&d);
    }
}

char editorProcessKeypress()
{
    char c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        return 'q';

    case CTRL_KEY('s'):
        writeMapToBinary(editorCharUnorderMap, fileName);
        return 'x';

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        // CKernel::Get()->printPosition(E.cx);
        // CKernel::Get()->printPosition(E.cy);
        return 'x';
    default:
        // editorCharUnorderMap.insert(EEC.pos,EEC);
        EEC.c = c;
        editorCharUnorderMap[EEC.pos] = EEC;
        return 'x';
    }
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

struct object *cmEdit(struct object *args)
{
    initEditor();

    //fileName = (car(args))->string; // write lines to buffer

    if(fileExists(fileName)){
        displayFileContent(fileName);
    }

    outStream.open(fileName, ios::out);

    while (1)
    {
        if (editorProcessKeypress() == 'q')
        {
            break;
        }
    }

    //CKernel::Get()->clear(); // if not clear,the editor's text will display

    //CKernel::Get()->restoreMode();

    //return NIL;
    return NULL;
}