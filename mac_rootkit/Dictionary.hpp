#ifndef __DICTIONARY_HPP_
#define __DICTIONARY_HPP_

#include "Array.hpp"

class Dictionary
{
	public:
		Dictionary() { }

		~Dictionary() { }

		void* get(char *key)
		{
			for(int i = 0; i < keys.getSize(); i++)
			{
				char *k = keys.get(i);

				if(strcmp(k, key) == 0)
				{
					return values.get(i);
				}
			}
		}

		int find(char *key)
		{
			for(int i = 0; i < keys.getSize(); i++)
			{
				char *k = keys.get(i);

				if(strcmp(k, key) == 0)
				{
					return i;
				}
			}

			return -1;
		}

		void set(char *key, void *value)
		{
			int index;

			if((index = find(key)) != -1)
			{
				values.set(value, i);

				return;
			}

			keys.add(key);
			values.add(value);
		}

	private:
		Array<char*> keys;
		Array<void*> values;
};

#endif