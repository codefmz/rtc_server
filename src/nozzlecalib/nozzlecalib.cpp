#include "nozzlecalib.h"
#include "rtc_utils.h"
#include "plog/Log.h"

#define LIGHT_SET 30            //补光灯亮度设置
#define DEV_NAME "/dev/video2"
#define NOZZLE_NUM 6

int NozzleCalib::nozzleOffsetPre(int nozzleT, int posX, int posY, int posZ)
{
    // 换头
    std::string nozzleTStr = "T" + std::to_string(nozzleT);
    int ret = mKlipper->sendCmd(nozzleTStr, true);
    if (ret != 0) {
        PLOGE << "send " << nozzleTStr << "failed, ret = " << ret;
        return ret;
    }

    ret = preGCodeAction(nozzleT == 0);
    if (ret != 0) {
        PLOGE << "Failed to pre gcode action, ret = " << ret;
        return ret;
    }

    std::string cmdX = "G1 X" + std::to_string(posX) + "F12000";
    ret = mKlipper->sendCmd(cmdX, true);
    if (ret != 0) {
        PLOGE << "Failed to G1 X" << posX << "F12000, ret = " << ret;
        return ret;
    }

    std::string cmdY = "G1 Y" + std::to_string(posY) + "F12000";
    ret = mKlipper->sendCmd(cmdY, true);
    if (ret != 0) {
        PLOGE << "Failed to G1 Y" << posY << "F12000, ret = " << ret;
        return ret;
    }

    return 0;
}

int NozzleCalib::nozzleOffsetPost(int nozzleT)
{
    return postGCodeAction(nozzleT == NOZZLE_NUM - 1);
}

int NozzleCalib::nozzleRt(int count, int posX, int posY, int posZ)
{
    int ret;
    if (count == 0) {
        ret = preGCodeAction(true);
        if (ret != 0) {
            PLOGE << "Failed to pre gcode action, ret = " << ret;
            return ret;
        }
    }

    std::string cmdX = "G1 X" + std::to_string(posX) + "F12000";
    ret = mKlipper->sendCmd(cmdX, true);
    if (ret != 0) {
        PLOGE << "Failed to G1 X" << posX << "F12000, ret = " << ret;
        return ret;
    }

    std::string cmdY = "G1 Y" + std::to_string(posY) + "F12000";
    ret = mKlipper->sendCmd(cmdY, true);
    if (ret != 0) {
        PLOGE << "Failed to G1 Y" << posY << "F12000, ret = " << ret;
        return ret;
    }

    if (count == 8) {
        return postGCodeAction(true);
    }

    return 0;
}

int NozzleCalib::preGCodeAction(bool isOpen)
{
    int ret;

    if (isOpen) {
        ret = Utils::setLight(DEV_NAME, LIGHT_SET);
        if(ret != 0){
            PLOGE << "Failed to set light, ret = " << ret;
            return ret;
        }
    }

    auto homeAxes = mKlipper->getHomeAxes();
    if (homeAxes!= "xyz") {
        ret = mKlipper->sendCmd("G28", true);
        if (ret != 0) {
            PLOGE << "Failed to home axes, ret = " << ret;
            return ret;
        }
    }

    //移动到安全位置 BOX_MOVE_TO_SAFE_POS
    ret = mKlipper->sendCmd("BOX_MOVE_TO_SAFE_POS", true);
    if (ret != 0) {
        PLOGE << "send BOX_MOVE_TO_SAFE_POS failed, ret = ." << ret;
        return ret;
    }

    //移动到挤出位置
    ret = mKlipper->sendCmd("BOX_GO_TO_EXTRUDE_POS", true);
    if (ret != 0) {
        PLOGE << "send BOX_GO_TO_EXTRUDE_POS failed, ret =." << ret;
        return ret;
    }

    ret = mKlipper->sendCmd("BOX_NOZZLE_CLEAN", true);
    if (ret != 0) {
        PLOGE << "Failed to nozzle offset pre, ret = " << ret;
        return ret;
    }

    ret = mKlipper->sendCmd("BOX_MOVE_TO_SAFE_POS", true);
    if (ret != 0) {
        PLOGE << "Failed to move to safe pos, ret = " << ret;
        return ret;
    }

    ret = mKlipper->sendCmd("G1 Z10 F6000", true);
    if (ret != 0) {
        PLOGE << "Failed to G1 Z10 F6000, ret = " << ret;
        return ret;
    }

    ret = mKlipper->sendCmd("SET_LIMITS", true);
    if (ret != 0) {
        PLOGE << "Failed to set limits, ret = " << ret;
        return ret;
    }

    return 0;
}

int NozzleCalib::postGCodeAction(bool isClose)
{
    int ret;

    if (isClose) {
        ret = Utils::setLight(DEV_NAME, 0);
        if (ret != 0) {
            PLOGE << "Failed to set light, ret = " << ret;
            return ret;
        }
    }

    ret = mKlipper->sendCmd("RESTORE_LIMITS", true);
    if (ret != 0) {
        PLOGE << "Failed to restore limits, ret = " << ret;
        return ret;
    }

    ret = mKlipper->sendCmd("BOX_MOVE_TO_SAFE_POS", true);
    if (ret != 0) {
        PLOGE << "Failed to move to safe pos, ret = " << ret;
        return ret;
    }

    std::string cmd = "M104 T" + std::to_string(0) + " S0";
    ret = mKlipper->sendCmd(cmd, true);
    if (ret != 0) {
        PLOGE << "Failed to set temp to 0, ret = " << ret;
        return ret;
    }

    return ret;
}
