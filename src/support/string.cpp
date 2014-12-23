#include <iostream>
#include <string>
#include <sstream>
#include "string.h"
using namespace std;


namespace str {
    bool startswith(const std::string& s, const std::string& w) {
        /*  Returns true if s stars with w.
         */
        return (s.compare(0, w.length(), w) == 0);
    }


    bool isnum(const std::string& s) {
        /*  Returns true if s contains only numerical characters.
         *  Regex equvallent: `^[0-9]+$`
         */
        bool num = false;
        for (int i = 0; i < s.size(); ++i) {
            switch (s[i]) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    num = true;
                    break;
                default:
                    num = false;
            }
            if (!num) break;
        }
        return num;
    }


    string sub(const string& s, int b, int e) {
        /*  Returns substring of s.
         *  If only s is passed, returns copy of s.
         */
        if (b == 0 and e == -1) return string(s);

        ostringstream part;
        part.str("");

        if (e < 0) { e = (s.size() + e + 1); }
        cout << b << ", " << e << ": `";

        for (; b < s.size() and b < e; ++b) { part << s[b]; }

        cout << part.str() << "`" << endl;

        return part.str();
    }

    string chunk(const string& s) {
        /*  Returns part of the string until first whitespace from left side.
         */
        ostringstream chnk;
        for (int i = 0; i < s.size(); ++i) {
            if (s[i] == *" " or s[i] == *"\t" or s[i] == *"\v" or s[i] == *"\n") break;
            chnk << s[i];
        }
        return chnk.str();
    }


    string lstrip(const string& s) {
        /*  Removes whitespace from left side of the string.
         */
        int i = 0;
        while (i < s.size()) {
            if (s[i] == *" " or s[i] == *"\t" or s[i] == *"\v" or s[i] == *"\n") {
                ++i;
            } else {
                break;
            }
        }
        return sub(s, i);
    }


    unsigned lshare(const string& s, const string& w) {
        unsigned share = 0;
        for (int i = 0; i < s.size() and i < w.size(); ++i) {
            if (s[i] == w[i]) {
                ++share;
            } else {
                break;
            }
        }
        return share;
    }
};
