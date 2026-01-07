#include "CNROM.h"

CNROM::CNROM()
{
	MapperBase::SetPrgPageSize(0x8000);
	MapperBase::SetChrPageSize(0x2000);
}

void CNROM::Serialize(Serializer& serializer) {

}

void CNROM::Deserialize(Serializer& serializer) {

}