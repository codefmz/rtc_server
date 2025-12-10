#ifndef _NOZZLE_CALIB_H_
#define _NOZZLE_CALIB_H_

#include "Klipper.h"

class NozzleCalib {
public:
    NozzleCalib(std::shared_ptr<Klipper> &klipper) : mKlipper(klipper) {
        
    };
    ~NozzleCalib() = default;

    int nozzleOffsetPre(int nozzleT, int posX, int posY, int posZ);

    int nozzleOffsetPost(int nozzleT);

    int nozzleRt(int count, int posX, int posY, int posZ);

private:
    int preGCodeAction(bool isOpen);

    int postGCodeAction(bool isClose);

private:
    std::shared_ptr<Klipper> mKlipper;
};

#endif