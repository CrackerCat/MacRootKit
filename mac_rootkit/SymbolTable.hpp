#ifndef __SYMBOL_TABLE_HPP_
#define __SYMBOL_TABLE_HPP_

#include <sys/types.h>

#include "Symbol.hpp"
#include "MachO.hpp"

class Symbol;
class MachO;

class SymbolTable
{
	public:
		SymbolTable();

		SymbolTable(struct nlist_64 *symtab, uint32_t nsyms, char *strtab, size_t strsize);

		Array<Symbol*> getAllSymbols() { return symbol_table; }

		Symbol* getSymbolByName(char *name);

		Symbol* getSymbolByAddress(mach_vm_address_t address);

		Symbol* getSymbolByOffset(off_t offset);

		void addSymbol(Symbol *symbol) { symbolTable.add(symbol; )}

		void removeSymbol(Symbol *symbol) { symbolTable.remove(symbol); }

	private:
		Array<Symbol*> symbolTable;

		struct nlist_64 *symtab;
		
		uint32_t nsyms;

		char *strtab;

		size_t strsize;
}

#endif