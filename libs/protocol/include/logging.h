#ifndef LOGGING_H
#define LOGGING_H

namespace logging{
    //easly redirectable to a file. I think it's fine to leave that as a simple code swap option.
    void log(const QString& string){
        qInfo()<<string;
    }
}

#endif //LOGGING_H
