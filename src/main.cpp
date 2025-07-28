#include "LivePP/API/x64/LPP_API_x64_CPP.h"

#include "App.h"

const unsigned int SCREEN_WIDTH = 3840;
const unsigned int SCREEN_HEIGHT = 2160;

int main()
{
    lpp::LppDefaultAgent lppAgent = lpp::LppCreateDefaultAgent(nullptr, L"include/LivePP");

    if (!lpp::LppIsValidDefaultAgent(&lppAgent))
        return 1;

    lppAgent.EnableModule(lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr);

    App app(SCREEN_WIDTH, SCREEN_HEIGHT);

    app.Run();

    lpp::LppDestroyDefaultAgent(&lppAgent);

    return 0;
}




