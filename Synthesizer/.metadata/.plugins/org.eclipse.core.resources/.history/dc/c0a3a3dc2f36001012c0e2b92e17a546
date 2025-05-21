#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include "xuartps.h"
#include "xgpio.h"
#include "stdio.h"
#include "../audio.h"

#include "xgpio.h"
#include "xparameters.h"
#include "sleep.h"
#include "xscugic.h"
#include "xscutimer.h"

#include "arm_math.h"
#include "arm_const_structs.h"

#define I32increment(v, a) \
    do { \
        if ((a) > 0 && (v) > INT_MAX - (a)) \
            (v) = INT_MAX; \
        else if ((a) < 0 && (v) < INT_MIN - (a)) \
            (v) = INT_MIN; \
        else \
            (v) += (a); \
    } while (0)

#define I32decrement(v, a) I32increment((v), -(a))

class Button_Array{
public:
	enum RE_STATE{
			OFF_Changed,
			ON_Changed,
			ON_Stable,
			OFF_Stable
		};
	u8 BTNx_State(u8 idx);
	static Button_Array& instance(){ static Button_Array BTA; return BTA; }
private:
	Button_Array();
	u8 PS_[4]{0,0,0,0};
	u8 CS_[4]{0,0,0,0};
	XGpio BTNS;
};

class Switch_arr{
public:
	enum RE_STATE{
			OFF_Changed,
			ON_Changed,
			ON_Stable,
			OFF_Stable
		};
	u8 SWx_State(u8 sw);
	static Switch_arr& instance(){ static Switch_arr SWA; return SWA; }
private:
	Switch_arr();
	u8 PS_[2]{0,0};
	u8 CS_[2]{0,0};
	XGpio SW;
};

class UART{
public:
	static UART& instance(){ static UART uart; return uart; }
private:
	UART();
	XUartPs Uart_Ps;
};

class Rotary_enc{
public:
	enum RE_STATE{
		LEFT,
		RIGHT,
		BUTTON,
		IDLE
	};

	RE_STATE GetSate();
	u32 GetCounter(){return counter;}
	void SetCounter(u32 v){counter = v;}

	static Rotary_enc& instance(){static Rotary_enc RE; return RE; };
private:
	Rotary_enc();

	u32 Getvalue(){ return XGpio_DiscreteRead(&rotary, 1); }

	XGpio 	rotary;
	int 	counter{0};
	u8 		debounce{1};
	u32 	PS{0};
	u32 	CS{0};
};
#endif
