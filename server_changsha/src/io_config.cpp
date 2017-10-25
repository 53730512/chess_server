

#include <string>
#include <map>
#include "io_handler.h"
////////////////////////////////////////////////////////////////////////////////
static std::map<io::stringc, io::stringc> key_value;
////////////////////////////////////////////////////////////////////////////////
namespace config{
////////////////////////////////////////////////////////////////////////////////
static io::stringc readfile(const char *filename)
{
    io::stringc data;
    FILE *fpread = fopen(filename, "r");
    if (!fpread){
        return data;
    }
    while (!feof(fpread)){
        char buffer[8192];
        size_t n = fread(buffer, 1, sizeof(buffer), fpread);
        data.append(buffer, n);
    }
    fclose(fpread);
    return std::move(data);
}
////////////////////////////////////////////////////////////////////////////////
static bool parseline(const io::stringc &data)
{
    io::stringc line;
    bool in_notes = false;
    std::vector<std::string> key_line;
    for (size_t i = 0; i < data.size(); i++){
        char c = data[i];
        if (c == '#'){
            in_notes = true;
            continue;
        } else if (c == '\n'){
            in_notes = false;
            if (!line.empty()){
                key_line.push_back(line);
                line.clear();
            }
            continue;
        } else if (in_notes){
            continue;
        }
        if (c == ' ' || !http::is_char(c) || http::is_ctl(c))
            continue;
        line += c; //ÓÐÐ§×Ö·û
    }
    for (size_t i = 0; i < key_line.size(); i++){
        size_t pos = key_line[i].find('=');
        if (pos == std::string::npos){
            continue;
        }
        io::stringc key = key_line[i].substr(0, pos).c_str();
        io::stringc value = key_line[i].substr(pos + 1).c_str();
        key_value[key] = value;
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool load(const char *filename)
{
    io::stringc data = readfile(filename);
    return (data.empty() ? false : parseline(data));
}
const char* get(const char *name)
{
    auto iter = key_value.find(name);
    return (iter == key_value.end() ? 0 : iter->second.c_str());
}
////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////
