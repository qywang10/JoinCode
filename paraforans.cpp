#include <iostream>
#include <fstream>
#include <cstring>    // memset

#include <typeinfo>

using namespace std;

// ---- get cpu info ---- //
string getCpuInfo()
{
    bool flag=false;
    ifstream fp;
    fp.open("/proc/cpuinfo");
    string sztest,ans;
    while(!flag){
        // fp>>sztest;
        getline(fp,sztest);
        // cout<<"fp="<<sztest<<endl;
        if(sztest.substr(0,10)=="model name"){
            // cout<<sztest<<endl;
            flag=true;
        }
    }
    fp.close();
    
    ans=sztest.substr(13,sztest.size());
    return ans;
}

// ---- get cache info ---- //
int getCacheInfo(int flag){
    // /sys/devices/system/cpu/cpu0/cache/index0/size
    // bool flag=false;
    string l1d,l1i,l2,l3;
    int ans;
    ifstream fl1d,fl1i,fl2,fl3;
    switch(flag){
        case 0:
            fl1d.open("/sys/devices/system/cpu/cpu0/cache/index0/size");    // L1 data cache
            getline(fl1d,l1d);
            fl1d.close();
            // cout<<"l1d="<<l1d<<"\n";
            ans=stoi(l1d);
            // int dat=stoi(l1d);
            // printf("type=%s\t",typeid(9).name());
            // cout<<"stoi="<<dat<<endl;
            // break;
            return ans;
        case 1:
            fl1i.open("/sys/devices/system/cpu/cpu0/cache/index1/size");    // L1 instruction cache
            getline(fl1i,l1i);
            fl1i.close();
            // cout<<"l1i="<<l1i<<"\n";
            ans=stoi(l1i);
            // break;
            return ans;
        case 2:
            fl2.open("/sys/devices/system/cpu/cpu0/cache/index2/size");    // L2 cache
            getline(fl2,l2);
            fl2.close();
            // cout<<"l2="<<l2<<"\n";
            ans=stoi(l2);
            // break;
            return ans;
        case 3:
            fl3.open("/sys/devices/system/cpu/cpu0/cache/index3/size");    // L3 cache
            getline(fl3,l3);
            fl3.close();
            // cout<<"l3="<<l3<<"\n";
            ans=stoi(l3);
            // break;
            return ans;
        default:
            cout<<"Error number\n";
            // return "ERROR";
            return -1;
    }
}
// int main(){
//     // getCpuInfo();
//     cout<<getCacheInfo(0)<<endl;
//     cout<<getCacheInfo(1)<<endl;
//     cout<<getCacheInfo(2)<<endl;
//     cout<<getCacheInfo(3)<<endl;
//     return 0;
// }
