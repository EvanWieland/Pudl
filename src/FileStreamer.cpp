#include "FileStreamer.h"

std::fstream *Pudl::FileStreamer::FileStream = nullptr;

void Pudl::FileStreamer::OpenFileStream(const char *FilePath) {
    FileStream = new std::fstream(FilePath);
}

void Pudl::FileStreamer::CloseFileStream() {
    FileStream->close();
    FileStream = nullptr;
}

bool Pudl::FileStreamer::InREPL() {
    return FileStream == nullptr || !FileStream->is_open();
}

int Pudl::FileStreamer::GetChar() {
    return FileStream->get();
}
