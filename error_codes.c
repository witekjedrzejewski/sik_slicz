#include "error_codes.h"

static char* err_messages[] = {
	"",
	"No port",
	"No receiver data",
	"No vlans",
	"Wrong receiver address",
	"Vlan out of bounds",
	"Tagged vlan duplicated",
	"More than one untagged vlan",
	"Socket setup failed",
	"Too many open ports"
};

char* get_error_message(int error_code) {
	return err_messages[error_code];
}