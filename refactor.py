import os
import re

parse_file = "src/parse.cpp"
senddata_file = "src/senddata.cpp"

with open(parse_file, "r") as f:
    parse = f.read()

parse = parse.replace("using namespace std;\n", "")

parse = parse.replace(
    'cInfo.r_filename = parseWord[1];',
    '''if (parseWord[1].find("..") != std::string::npos) parseWord[1] = "";
\t\tcInfo.r_filename = parseWord[1];'''
)

strtok_old = """\t\tpbuffer = new char[cInfo.r_firstline.size() + 1];
\t\tstd::copy(cInfo.r_firstline.begin(), cInfo.r_firstline.end(), pbuffer);
\t\tpbuffer[cInfo.r_firstline.size()] = '\\0';

\t\tp=strtok (pbuffer," /");
\t\tparseWord[0].assign(p);
\t\twhile (i < 3) {
\t\t\t  p=strtok(NULL," ");
\t\t\t  parseWord[i].assign(p);
\t\t\t  i++;
\t\t}"""
strtok_new = """\t\tint pos1 = cInfo.r_firstline.find(' ');
\t\tif (pos1 != std::string::npos) {
\t\t\tparseWord[0] = cInfo.r_firstline.substr(0, pos1);
\t\t\tint pos2 = cInfo.r_firstline.find(' ', pos1 + 1);
\t\t\tif (pos2 != std::string::npos) {
\t\t\t\tstd::string path = cInfo.r_firstline.substr(pos1 + 1, pos2 - pos1 - 1);
\t\t\t\tif (!path.empty() && path[0] == '/') path = path.substr(1);
\t\t\t\tparseWord[1] = path;
\t\t\t\tparseWord[2] = cInfo.r_firstline.substr(pos2 + 1);
\t\t\t}
\t\t}"""
parse = parse.replace(strtok_old, strtok_new)

parse = parse.replace("string parseWord[3];", "std::string parseWord[3];")
parse = parse.replace("delete [] pbuffer;\n\t\tpbuffer = NULL;", "")
parse = parse.replace("char *p,*pbuffer;", "char *p;")

parse = parse.replace("bool Parse::fileExists(char *filename)", "bool Parse::fileExists(const std::string& filename)")
parse = parse.replace("stat(filename, &filenamecheck)", "stat(filename.c_str(), &filenamecheck)")
parse = parse.replace("ifstream file;", "std::ifstream file;")

fname_old = """\t\tchar * fname = new char[cInfo.r_filename.size() + 1];
\t\tstd::copy(cInfo.r_filename.begin(), cInfo.r_filename.end(), fname);
\t\tfname[cInfo.r_filename.size()] = '\\0';"""
parse = parse.replace(fname_old, "\t\tstd::string fname = cInfo.r_filename;")
parse = parse.replace("delete [] fname;\n\t\t\tfname = NULL;", "")
parse = parse.replace("delete [] fname;\n\t\tfname = NULL;", "")
parse = parse.replace("file.open(fname);", "file.open(fname.c_str());")

parse = re.sub(r'\bstring\b', 'std::string', parse)
parse = parse.replace("std::std::string", "std::string")
parse = re.sub(r'\bifstream\b', 'std::ifstream', parse)
parse = re.sub(r'\bios\b', 'std::ios', parse)
parse = re.sub(r'\btransform\b', 'std::transform', parse)

with open(parse_file, "w") as f:
    f.write(parse)

with open(senddata_file, "r") as f:
    senddata = f.read()

senddata = senddata.replace("using namespace std;\n", "")
senddata = senddata.replace("char * fname = new char[c.r_filename.size() + 1];\n\t\tstd::copy(c.r_filename.begin(), c.r_filename.end(), fname);\n\t\tfname[c.r_filename.size()] = '\\0';\n\t\tstat(fname,&buf);", "std::string fname = c.r_filename;\n\t\tstat(fname.c_str(),&buf);")
senddata = senddata.replace("\t\tdelete[] fname;\n\t\tfname = NULL;\n", "")

senddata = senddata.replace("char * dirname = new char[dir.size() + 1];\n\tstd::copy(dir.begin(), dir.end(), dirname);\n\tdirname[dir.size()] = '\\0';\n\td=opendir(dirname);", "d=opendir(dir.c_str());")
senddata = senddata.replace("\t\tdelete [] dirname;\n\t\tdirname = NULL;\n", "")

senddata = senddata.replace("char *readblock;", "std::vector<char> readblock;")
senddata = senddata.replace("readblock = new char [size];", "readblock.resize(size);")
senddata = senddata.replace("file.read(readblock, size);", "file.read(readblock.data(), size);")
senddata = senddata.replace("send(c.r_acceptid, readblock, size, 0)", "send(c.r_acceptid, readblock.data(), size, 0)")
senddata = senddata.replace("\t\tdelete [] readblock;\n", "")

senddata = re.sub(r'\bstring\b', 'std::string', senddata)
senddata = senddata.replace("std::std::string", "std::string")
senddata = re.sub(r'\bvector\b', 'std::vector', senddata)
senddata = re.sub(r'\bifstream\b', 'std::ifstream', senddata)
senddata = re.sub(r'\bofstream\b', 'std::ofstream', senddata)
senddata = re.sub(r'\bcout\b', 'std::cout', senddata)
senddata = re.sub(r'\bendl\b', 'std::endl', senddata)
senddata = re.sub(r'\bios\b', 'std::ios', senddata)

with open(senddata_file, "w") as f:
    f.write(senddata)

print("Refactoring complete.")
