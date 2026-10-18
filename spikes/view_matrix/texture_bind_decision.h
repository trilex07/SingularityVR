#pragma once

// One invocation only. Never stores a COM pointer or skips a native bind.
namespace texturebind {
struct Decision {
    bool enabled=false,known=false,value=false;
    unsigned requests=0,executions=0;
    explicit Decision(bool on):enabled(on){}
    template<class Classify> bool Get(Classify classify) {
        ++requests;
        if(!enabled||!known){++executions;value=classify();known=true;}
        return value;
    }
};
}
