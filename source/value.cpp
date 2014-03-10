#include "value.h"

void CValue::updateHash (SHA1& hash) {

	if (this == NULL)
		hash.update ("null");
	else {
		hash.update ("class:");
		hash.update (typeid (*this).name());
		hash.update (":args:");	
		updateHashArgs (hash);
	}
	
}
