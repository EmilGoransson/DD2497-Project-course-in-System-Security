#include "altc/altio.h"
#include "altc/string.h"
#include "s3k/s3k.h"

#include <stdint.h>

#include "../../tutorial-commons/utils.h"

int main(void)
{
	alt_puts("1> starting");
	s3k_msg_t msg;
	s3k_reply_t reply;
	// Set the reply message to "appB.bin"o	
	memcpy(msg.data, "appB.bin", 9);
	reply = send_receive_forever(APP_1_CAP_SOCKET, msg);
	alt_puts("1> end");
}
// Hur kan de 2 programmen existera i samma memspace utan att skriva över varandra?
