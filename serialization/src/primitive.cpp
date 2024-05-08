#include "../include/primitive.h"
#include "../include/core.h"
#include <stdint.h>

#include <memory>

namespace ObjectModel
{

	Primitive::Primitive()
	{
		size += sizeof type;
	}


	void Primitive::pack(std::vector<uint8_t>& buffer, int16_t& iterator) const
	{
		 Core::encode<uint8_t>(buffer, iterator, wrapper);
                 Core::encode<int16_t>(buffer, iterator, nameLength);
                 Core::encode<std::string>(buffer, iterator, name);
                 Core::encode<uint8_t>(buffer, iterator, type);

                 if (data) {
                   Core::encode(buffer, iterator, *(*data));
                   Core::encode<int32_t>(buffer, iterator, static_cast<int32_t>((*data)->size()));
                }
		
	}


	static Primitive Primitive::unpack(const std::vector<uint8_t>& buffer, int16_t& it)
	{
             Primitive p;

             p.wrapper = Core::decode<uint8_t>(buffer, it);
             p.nameLength = Core::decode<int16_t>(buffer, it);
             p.name = Core::decode<std::string>(buffer, it);
             p.type = Core::decode<uint8_t>(buffer, it);

             int dataSize = p.getTypeSize(static_cast<Type>(p.type));
             p.data = std::make_unique<std::vector<uint8_t>>(dataSize);

             Core::decode(buffer, it, *p.data);
             p.size = Core::decode<int32_t>(buffer, it);

             return p;		
	}


      std::vector<uint8_t> getData() const {
          if (data) 
              return *(*data);
          return {};
      }
}
