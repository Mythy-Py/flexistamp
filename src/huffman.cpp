#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>
#pragma once

// HOWTO -> compile with std 17
//  g++ *.cpp -std==c++17 -o prog
//  Klammern sind optional
//  ./prog -c infile (output) (-v)   Compress infile
//  ./prog -e compressedfile (output) (-v)   Decompress compressedfile
template <typename T>
struct Node
{
    T chr = 0;
    unsigned int count = 0;
    std::string binary;
    Node *left = nullptr;
    Node *right = nullptr;

    void insert(T add, unsigned int qty = 0)
    {
        Node *set = this;
        while (set)
        {
            set->count += qty;
            if (!set->left)
            {
                set->left = new Node{add, qty};
                break;
            }
            else if (set->left and !set->right)
            {
                set->right = new Node{add, qty};
                break;
            }
            else if (set->left and set->right)
            {
                if (set->left->count > set->right->count)
                {
                    if (set->right->chr)
                    {
                        Node *temp = new Node{0, qty + set->right->count};
                        std::swap(temp, set->right);
                        set->right->left = temp;
                    }
                    set = set->right;
                }
                else
                {
                    if (set->left->chr)
                    {
                        Node *temp = new Node{0, qty + set->left->count};
                        std::swap(temp, set->left);
                        set->left->right = temp;
                    }
                    set = set->left;
                }
            }
        }
    }

    void print(std::string binary = "")
    {
        if (chr)
        {
            // Den Kram kurz umdrehen:
            // std::reverse(binary.begin(),binary.end());
            std::cout << "Char: " << chr << " Code: " << binary << std::endl;
        }
        if (left)
        {
            left->print(binary + "0");
        }
        if (right)
        {
            right->print(binary + "1");
        }
    }

    void genTable(std::vector<std::pair<T, std::string>> &vector, std::string code = "")
    {
        if (chr)
        {
            vector.emplace_back(std::make_pair(chr, code));
        }
        if (left)
        {
            left->genTable(vector, code + "0");
        }
        if (right)
        {
            right->genTable(vector, code + "1");
        }
    }

    void clear()
    {
        if (left)
            delete left;
        if (right)
            delete right;
    }
    ~Node()
    {
        if (left)
            delete left;
        if (right)
            delete right;
    }
};

class Huffman
{
private:
    Node<char> start;                                // Der Star der Show : der Binary Tree
    std::vector<std::pair<char, unsigned int>> vec;  // Der Vector der die Anzahl der Sequenzen beinhaltet
    std::vector<std::pair<char, std::string>> table; // Die Vector der die Tabelle von Chars und die Stelle im Vec Beinhaltet
    // functions
    std::vector<std::pair<char, unsigned int>>::iterator search(char search) // Die funktion Findet den Char im Vector,zählt ihn hoch oder erstellt ihn neu
    {
        for (auto it = vec.begin(); it != vec.end(); ++it)
        {
            if (it->first == search)
                return it;
        }
        return vec.end();
    }

    std::string *find(char search) // Gibt den Pointer auf den String zurück
    {
        for (auto it = table.begin(); it != table.end(); ++it)
        {
            if (it->first == search)
                return &it->second;
        }
        return nullptr;
    }

    void sort() // Sortiert den Vector nach Anzahl der Buchstaben (am meisten Genutzter Buchstabe ist vorne)
    {
        std::sort(vec.begin(), vec.end(), [](std::pair<char, unsigned int> pair1, std::pair<char, unsigned int> pair2)
                  { return pair1.second > pair2.second; });
    }

    void makeTree() // Erstellt aus dem Vector einen Baum.
    {
        for (auto &chr : vec)
        {
            start.insert(chr.first, chr.second);
        }
        start.genTable(table);
    }

    void clean()
    {
        if(!table.empty())
            table.clear();
        if(!vec.empty())
            table.clear();
    }

public:
    explicit Huffman() = default;
    ~Huffman() = default;

    void readFile(std::istream& file) // Liest die zu Komprimierende Datei ein
    {
        char buff;
        size_t count = 0;
        vec.reserve(25);


        if (!file.good())
        {
            std::cerr << "File not Found or good!\n";
            return;
        }

        while (file.good())
        {
            ++count;
            file.read(&buff, sizeof(buff));
            auto found = search(buff);
            if (found != vec.end())
            {
                found->second++;
            }
            else
            {
                vec.emplace_back(std::make_pair(buff, 1));
            }
            if (count % 100 == 0)
            {
                sort();
            }
        }
        sort();
        makeTree();
    }

    void readString(std::string &input) // Liest den String ein
    {
        size_t count = 0;
        vec.reserve(25);
        for (auto &chr : input)
        {
            ++count;
            auto found = search(chr);
            if (found != vec.end())
            {
                found->second++;
            }
            else
            {
                vec.emplace_back(std::make_pair(chr, 0));
            }
            if (count % 100 == 0)
            {
                sort();
            }
        }

        sort();
        makeTree();
    }

    void compress(std::istream& file, std::ostream& compressed)
    {
        // std::ofstream hexin ("hexin.txt"); //DEBUG-Hex
        clean();
        readFile(file);

        if (vec.size() < 2 && table.size() < 2 && !file.eof())
        {
            std::cerr << "Can´t Compress File!\n";
            return;
        }

        // Aufbau der Binärdatei:
        //  ersten 32 Bit N einträge;
        //  N * 4 Bit an einträgen;
        //  Bis Ende der Binärdatei > Sequenzen aus dem Baum;

        //compressed = std::ofstream(outname, std::ios::binary);
        const int size = vec.size();
        compressed.write(reinterpret_cast<const char *>(&size), sizeof(size));

        for (auto &pair : vec)
            compressed.write(reinterpret_cast<char*>(&pair), sizeof(pair));

        file.clear();
        file.seekg(0);
        char buff = 0;
        char write = 0;
        unsigned char bytepos = 0;

        while (file.good())
        {
            file.read(reinterpret_cast<char*>(&buff), sizeof(buff));

            std::string *sequence = find(buff);
            if (!sequence)
            {
                std::cerr << "FATAL ERROR, Sequence not found!\n";
                std::abort();
            }

            for (auto &bin : *sequence)
            {
                write <<= 1;
                if (bin == '1')
                    write |= 0b1;

                if (++bytepos == 8)
                {
                    // in ausgabe
                    // hexin << std::hex << (write ? (int)write : 0) << std::dec << " ";
                    compressed.write(&write, 1);
                    write = 0; 
                    bytepos = 0;
                }
            }
        }
        if (size && bytepos) // wenn noch ungeschriebene Bits vorhanden sind:
            compressed << write;
    }

    void decompress(std::istream& file, std::ostream& output)
    {
        clean();
        // std::ofstream hexout ("hexout.txt"); // DEBUG HEX
        // std::ifstream file (filename, std::ios::binary);
        // std::ofstream output(outname, std::ios::binary);

        int entries = 0;
        file.read((char *)&entries, sizeof(entries));

        std::pair<char, unsigned int> buff;

        for (int i = 0; i < entries; ++i)
        {
            file.read((char *)&buff, sizeof(buff));
            vec.emplace_back(buff);
        }

        for (auto &e : vec)
        {
            start.insert(e.first, e.second);
        }

        char seq;
        unsigned int decode = 0;
        Node<char> *pos = &start;

        while (!file.eof())
        {
            file.read(&seq, sizeof(seq));
            // out-ausgabe
            // hexout << std::hex << (int)seq << std::dec << " ";
            for (int i = (sizeof(char) * 8) - 1; i >= 0; --i)
            {
                //if (decode >= start.count-1)
                if (start.count == 0)
                {
                    return;
                }

                if ((seq >> i) & 1)
                {
                    pos = pos->right;
                }
                else
                {
                    pos = pos->left;
                }

                if (pos->chr)
                {
                    output << pos->chr;
                    pos = &start;
                    --start.count;
                    //decode++;
                }
            }
        }
    }

    void printTree() // Gibt den Binärbaum aus
    {
        std::cout << "Tree Size: " << start.count << "\n";
        start.print();
    }

    void printVec() // Gibt den Vector mit Buchstaben und Anzahl aus
    {
        std::cout << "Vector Size: " << vec.size() << "\n";
        for (auto &pair : vec)
            std::cout << "Char: " << pair.first << "\tSize: " << pair.second << "\n";
    }

    void printTable() // Gibt den Vector aus der Speichert welcher Buchstabe welche Sequenz hat
    {
        std::cout << "Table Size: " << table.size() << "\n";
        std::sort(table.begin(), table.end(), [](std::pair<char, std::string> in1, std::pair<char, std::string> in2)
                  { return in1.second < in2.second; });
        std::sort(table.begin(), table.end(), []( std::pair<char, std::string> in1, std::pair<char, std::string> in2)
                  { return in1.second.length() < in2.second.length(); });
        for (auto &pair : table)
            std::cout << "Char: " << pair.first << "\tTable: " << pair.second << "\n";
    }
};
