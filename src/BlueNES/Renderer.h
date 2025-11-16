#pragma once
class Core;
class NesPPU;

class Renderer
{
public:
	virtual void Initialize(NesPPU* ppu) = 0;
	virtual uint8_t GetScrollX() = 0;
	virtual void SetScrollX(uint8_t value) = 0;
	virtual uint8_t GetScrollY() = 0;
	virtual void SetScrollY(uint8_t value) = 0;
	virtual const std::array<uint32_t, 256 * 240>& get_back_buffer() const = 0;
	virtual void reset() = 0;
	virtual bool isFrameComplete() = 0;
	virtual void clock() = 0;
	virtual void setFrameComplete(bool complete) = 0;
};