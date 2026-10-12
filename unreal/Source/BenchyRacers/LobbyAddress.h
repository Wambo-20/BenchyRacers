#pragma once
#include <string>

namespace BenchyLobby {
inline bool ParseIPv4(const std::string& Text,unsigned int (&Octets)[4]) {
    size_t At=0;
    for(int I=0;I<4;I++) {
        unsigned int Value=0;size_t Start=At;
        while(At<Text.size()&&Text[At]>='0'&&Text[At]<='9') {
            Value=Value*10+unsigned(Text[At++]-'0');if(Value>255||At-Start>3)return false;
        }
        if(At==Start||(At-Start>1&&Text[Start]=='0'))return false;
        Octets[I]=Value;
        if(I<3&&(At>=Text.size()||Text[At++]!='.'))return false;
    }
    return At==Text.size();
}
inline bool ShareableIPv4(const std::string& Text,bool Internet) {
    unsigned int A[4];if(!ParseIPv4(Text,A))return false;
    if(A[0]==0||A[0]==127||A[0]>=224||(A[0]==169&&A[1]==254)||A[3]==255)return false;
    if(!Internet)return true;
    if(A[0]==10||(A[0]==172&&A[1]>=16&&A[1]<=31)||(A[0]==192&&A[1]==168))return false;
    if(A[0]==100&&A[1]>=64&&A[1]<=127)return false;
    if(A[0]==192&&A[1]==0&&(A[2]==0||A[2]==2))return false;
    if(A[0]==198&&(A[1]==18||A[1]==19||(A[1]==51&&A[2]==100)))return false;
    if(A[0]==203&&A[1]==0&&A[2]==113)return false;
    return true;
}
inline std::string Endpoint(const std::string& IP,int Port) {
    return Port>0&&Port<=65535&&ShareableIPv4(IP,false)?IP+":"+std::to_string(Port):std::string();
}
}
