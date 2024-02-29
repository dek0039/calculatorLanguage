#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <Windows.h>
#include <map>
#include <filesystem>


std::vector<std::string> readFile(std::string file)
{
    std::vector<std::string> ret;

    std::ifstream f(file);
    std::string line;
    if (f.is_open()) {
        while (getline(f, line)) {
          if (!line.empty())
            ret.push_back(line);
            line.clear();
        }
        f.close();
    }

    return ret;
}

enum class OP
{
    NONE,
    ADD = '+',
    SUB = '-',
    DIV = '/',
    MULT = '*',
    EQUAL = '='
};

std::map<std::string, std::shared_ptr<int>> vars;
std::map <std::string, std::pair<int, OP>> varOps;
std::map <std::string, std::pair<std::shared_ptr<int>, OP>> varOpsQueued;

std::string removeSpacing(std::string str)
{
    std::string temp;

    for (const auto c : str)
        if (c != ' ' && c != '\n')
            temp += c;

    return temp;
}

std::string removeChar(std::string str, char ch)
{
    std::string temp;

    for (const auto c : str)
        if (c != ch)
            temp += c;

    return temp;
}

std::string removeString(std::string str, std::string str2)
{
    auto npos = str.find(str2);

    if (npos != std::string::npos)
        str = str.erase(npos, str2.length());

    return str;
}


OP getOperation(std::string line)
{
    for (const char c : line)
    {
        if (c == '+')
            return OP::ADD;
        else if (c == '-')
            return OP::SUB;
        else if (c == '*')
            return OP::MULT;
        else if (c == '/')
            return OP::DIV;
        else if (c == '=')
            return OP::EQUAL;
    }

    return OP::NONE;
}

bool isVarAddition(std::string line)
{
    if (auto pos = line.find("="); pos != std::string::npos)
    {
        auto left = line.substr(0, pos);
        auto right = removeSpacing(line.substr(pos + 1, line.length()));

        if (isdigit(right.at(0)))
            return false;
    }

    return true;
}


std::string extractVarName(std::string substr, bool isCalc = false)
{
    std::string temp;
    auto varNpos = substr.find(isCalc ? "calc" : "var");
    substr = substr.substr(isCalc ? varNpos + 4 : varNpos + 3, substr.length());
    for (const char c : substr)
    {
        if (c != ' ')
            temp += c;
    }

    return temp;
}

int extractValue(std::string varName, std::string substr)
{
    auto varNpos = substr.find(varName);
    std::string varRemoved = substr.substr(varNpos + varName.length(), substr.length());
    
    std::string temp;

    for (const char c : varRemoved)
    {
        if (isdigit(c))
        {
            temp += c;
        }
    }

    return std::stoull(temp.c_str());
}

void pushVarsIntoMap(std::string file)
{
    auto contents = readFile(file);
    for (const auto& line : contents)
    {
        if (line.empty())
            continue;

        if (line.find("var") != std::string::npos)
        {
            if (getOperation(line) == OP::EQUAL)
            {
                if (!isVarAddition(line))
                {
                    auto varNpos = line.find("var");
                    auto equalNpos = line.find("=");
                    auto substrVar = line.substr(varNpos, equalNpos);

                    vars[extractVarName(substrVar)] = std::make_shared<int>(extractValue(extractVarName(substrVar), line));
                }
            }
        }
    }
}

std::string getSecondVar(std::string substr, OP op)
{
   auto npos = substr.find((char)op);

   if (npos == std::string::npos)
       return "404";

   std::string left = removeString(removeSpacing(substr.substr(0, npos)), "calc");
   std::string right = removeChar(removeSpacing(substr.substr(npos, substr.length())), (char)op);

   if (isdigit(right.at(0)))
       return "404";

   return right;
}

void pushOpsIntoMap(std::string file)
{
    auto contents = readFile(file);
    for (const auto& line : contents)
    {
        if (line.empty())
            continue;

        if (line.find("calc") != std::string::npos)
        {
            if (auto op = getOperation(line); op != OP::NONE)
            {
                auto varNpos = line.find("calc");
                auto equalNpos = line.find((char)op);
                auto substrVar = line.substr(varNpos, equalNpos);

                if (vars.contains(getSecondVar(line, op))) {
                    varOpsQueued[extractVarName(substrVar, true)] = { vars[getSecondVar(line, op)], op };
                }
                else
                {
                    varOps[extractVarName(substrVar, true)] = { extractValue(extractVarName(substrVar, true), line), op };
                }
            }
        }        
    }
}

void printStatements(std::string file)
{
    auto contents = readFile(file);
    for (const auto& line : contents)
    {
        if (line.empty())
            continue;

        if (auto pos = line.find("print");pos != std::string::npos)
        {
            auto noSpaces = removeSpacing(line);

            auto right = noSpaces.substr(pos + 6, line.length());
            auto closing = right.find(")");
            auto inner = noSpaces.substr(pos + 6, closing);
            if (vars.contains(inner))
            {
                std::cout << inner << ": " << *vars[inner] << std::endl;
            }
        }
    }

}

void doCalc()
{
    for (const auto& [name, ctx] : varOps)
    {
        if (ctx.second == OP::ADD)
            *vars[name] += ctx.first;
        if (ctx.second == OP::SUB)
            *vars[name] -= ctx.first;
        if (ctx.second == OP::MULT)
            *vars[name] = *vars[name] * ctx.first;
        if (ctx.second == OP::DIV)
            *vars[name] = *vars[name] / ctx.first;
    }
    
    for (const auto& [name, ctx] : varOpsQueued)
    {
        if (ctx.second == OP::ADD)
           *vars[name] += *ctx.first;
        if (ctx.second == OP::SUB)
            *vars[name] -= *ctx.first;
        if (ctx.second == OP::MULT)
            *vars[name] = *vars[name] * *ctx.first;
        if (ctx.second == OP::DIV)
            *vars[name] = *vars[name] / *ctx.first;
        if (ctx.second == OP::EQUAL)
            *vars[name] = *ctx.first;
    }
}

int main(int argc, char* argv[])
{   
    AllocConsole();
   
    if (!std::filesystem::exists(argv[1])) {
        std::cout << "File does not exist!" << std::endl;
        return 1;
    }

    pushVarsIntoMap(argv[1]);
    pushOpsIntoMap(argv[1]);
    doCalc();

    printStatements(argv[1]);

    FreeConsole();
	return 0;
}	
