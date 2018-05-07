// experiment with coding in a way tolerant to enabling/disabling escception

#include <iostream>
#include <array>

static thread_local auto errors_outstanding = int(0);
static thread_local auto last_error = "";
#define ODD_NUMBER "odd number"
#define RAISE(err, val) ++errors_outstanding; last_error = err; return (val);
#define CHECKED(err) auto err = errors_outstanding;
#define ON_CHECK_ERROR(err) for(; errors_outstanding > err; errors_outstanding = err)
#define CHECK_RAISE(val, r) auto current_errors = errors_outstanding; r; if(current_errors < errors_outstanding){return val;}

auto mess(int i)-> const char* {
    return last_error;
}

auto gun(int i)-> int {
    if(i%2 == 0){
        return i;
    } else {
        RAISE(ODD_NUMBER, -1);
    }
}

auto fun(int i)-> int {
   return gun(i);
}

auto main()-> int {
    auto j = 0;
    CHECK_RAISE(-1, auto i = fun(j)); // return -1 if error escapes fun(), error continues escaping

    int k;
    CHECKED(err){
        k = fun(j);
    } ON_CHECK_ERROR(err){
        std::cerr << mess(err) << "\n";
    }

   return 0;
}
