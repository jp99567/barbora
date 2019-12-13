#include "OpenTherm.h"
#include <cstdint>
#include <chrono>
#include <cmath>
#include "Log.h"
#include "Pru.h"
#include "../pru/rpm_iface.h"

namespace {

constexpr uint32_t calc_parity(unsigned v, unsigned pos);

constexpr uint32_t calc_parity(unsigned v, unsigned pos)
{
	if(pos==0)
		return v & 1;

	return ((v & (1<<pos)) ? 1 : 0)
	+ calc_parity(v, pos-1);
}

constexpr uint32_t parity(uint32_t v)
{
	int par=0;
	for(int i=0; i<31; ++i){
		if(v & (1<<i)) ++par;
	}

	if(par&1)
		return v | (1<<31);
	else
		return v & ~(1<<31);
}

constexpr uint16_t float2f88(float v)
{
	return std::lround(v * 256);
}

}

OpenTherm::OpenTherm(std::shared_ptr<Pru> pru)
	:pru(pru)
	,rxMsg(std::make_shared<PruRxMsg>())
{
	pru->setOtCbk(rxMsg);

	thrd = std::thread([this]{
		    int id=0;
			while(not shutdown){
				auto lastTransmit = std::chrono::steady_clock::now();
				bool error = false;
				if(id==0){
					//Mrd-Srdack id=0 M: v16=03CA (00000011.11001010) f88=3.78906 S: v16=0320 (00000011.00100000) f88=3.125
					uint32_t txv = 0x03CA;
					auto rsp = transmit(parity(txv));
					LogINFO("ow transfer {:08X} id0 {:b}", parity(txv), rsp);
					if(rsp==0) error |= 1;
				}
				else if(id==1){
					// Mwr-Swrack id=1 M: v16=3900 (00111001.00000000) f88=57.0 S: v16=3900 (00111001.00000000) f88=57.0
					uint32_t txv = float2f88(60);
					txv |= 1 << 16;
					txv |= 0b00010000 << 24;
					auto rsp = transmit(parity(txv));
					LogINFO("ow transfer {:08X} {:08X} {:04X} id1 {:X}", txv, parity(txv), float2f88(60), rsp);
					if(rsp==0) error |= 1;
				}

				if(++id > 1)
					id=0;

				std::this_thread::sleep_until(lastTransmit + std::chrono::seconds( error ? 60 : 1));
			}
	});
}

OpenTherm::~OpenTherm()
{
	LogDBG("~OpenTherm");
	shutdown = true;
	thrd.join();
}

uint32_t OpenTherm::transmit(uint32_t frame)
{
	uint32_t data[2] = { pru::eCmdOtTransmit, parity(frame) };
	pru->send((uint8_t*)&data, sizeof(data));
	auto buf = rxMsg->wait(std::chrono::seconds(5));

	if(buf.empty()){
		LogERR("OpenTherm buf empty");
		return 0;
	}

	if(buf.size() != 2*sizeof(uint32_t)){
		LogERR("OpenTherm buf invalid size {}", buf.size());
		return 0;
	}

	auto pmsg = reinterpret_cast<uint32_t*>(&buf[0]);

	switch((pru::ResponseCode)pmsg[0])
	{
	case pru::ResponseCode::eOtBusError:
	case pru::ResponseCode::eOtFrameError:
		break;
	case pru::ResponseCode::eOtOk:
	{
		auto frame = pmsg[1];
		if(frame != parity(frame)){
			LogERR("ot rx error parity");
			return 0;
		}
		return frame;
	}
		break;
	default:
		break;
	}

	LogERR("ot rx error {}", pmsg[0]);
	return 0;
}