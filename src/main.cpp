#include <iostream>
#include <string>
#include "ui.h"
#include "calc.h"
#include "manager.h"

int main()
{
    int status = 1; // 表示全局状态
    UI ui(status);
    Manager manager(ui);
    ui.run();
    system("pause");
    return 0;
}
