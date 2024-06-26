#pragma once
#include "root.h"
#include <memory>
#include "core.h"


namespace ObjectModel
{
	class Primitive : public Root
	{
	private:
		uint8_t type = 0;
		std::unique_ptr<std::vector<uint8_t>> data;
                Primitive() = default;
	public:
		template<typename T>
		static std::unique_ptr<Primitive> create(std::string name, Type type, T value)
		{
			auto p = std::make_unique<Primitive>();
                        p->setName(name);
                        p->wrapper = static_cast<uint8_t>(Wrapper::PRIMITIVE);
                        p->type = static_cast<uint8_t>(type);
                        p->data = std::make_unique<std::vector<uint8_t>>(sizeof(T));

                        int16_t iterator = 0;
                        Core::encode<T>(*p->data, iterator, value);

                         p->size += static_cast<int32_t>(p->data->size());
                         return p;
		}

		void pack(std::vector<uint8_t>&, int16_t&);
		static Primitive unpack(const std::vector<uint8_t>&, int16_t&);

		std::vector<uint8_t> getData();

	};



}
