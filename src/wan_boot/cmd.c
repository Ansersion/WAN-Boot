#include "cmd.h"
#include "checksum.h"

const Cmd Hello = {
	.Name = "hello",
	.Opts = {"-h", NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

const Cmd Burn = {
	.Name = "burn",
	.Opts = {"-f", "-a", "-s", "-h", "-c", NULL, NULL, NULL}
};

const Cmd Startos = {
	.Name = "startos",
	.Opts = {"-h", NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

int GetCmd(uint8_t * msg, uint8_t * cmd)
{
	int k;
	if(!msg || !cmd) {
		return -1;
	}
#ifdef WANP
	Wan_Get_ReqCmd(msg, cmd);
#else
	for(k = 0; 1; k++) { // "-1": reserve 1 character for '\0'
		switch(msg[k]) {
			case ' ':
			case '\0':
			case '\r':
			case '\n':
				cmd[k] = '\0';
				break;
			default:
				cmd[k] = msg[k];
				break;
		}
		if('\0' == cmd[k]) break;
	}
#endif
	return 0;
}

int RespOK(uint8_t * buf, uint8_t * cmd, uint8_t * append_msg)
{
	if(!buf || !cmd) {
		return -1;
	}
	#ifdef WANP
	Wan_InitHeader(buf);
	Wan_Set_RespCmd(buf, cmd);
	Wan_Set_RespOK(buf);
	if(append_msg) {
		Wan_Set_RespAppendMsg(buf, append_msg);
	}
	Wan_Set_RespEndFlag(buf);
	#else
	// ignore response
	strcpy(buf, cmd);
	if(append_msg) {
		strcpy(buf + strlen(cmd), append_msg);
	}
	#endif
	return 0;
}

int RespErr(uint8_t * buf, uint8_t * cmd, uint8_t * reason)
{
	if(!buf || !cmd) {
		return -1;
	}
	#ifdef WANP
	Wan_InitHeader(buf);
	Wan_Set_RespCmd(buf, cmd);
	Wan_Set_RespErr(buf);
	if(reason) {
		Wan_Set_RespAppendMsg(buf, reason);
	}
	Wan_Set_RespEndFlag(buf);
	#else
	strncpy((char *)buf, (const char *)cmd, strlen((const char *)cmd));
	buf += strlen((const char *)cmd);
	strncpy((char *)buf, ": ", strlen(": "));
	buf += strlen(": ");
	strncpy((char *)buf, (const char *)reason, strlen((const char *)reason));
	#endif
	
	return 0;
}

int DoHello(uint8_t * msg)
{
	if(!msg) return -1;
	
#ifdef WANP
	msg += WAN_HEADER_SIZE;
#endif
	if(strstr((const char *)msg, "-h") != NULL) {
		return HELP_HELLO;
	}
	return 0;
}

int DoBurn(uint8_t * msg, uint8_t * kernel_name, uint8_t * addr, uint8_t * kernel_size, uint8_t * crc)
{
	char * save_ptr;
	char * ptr;
	*kernel_name = '\0';
	*addr = '\0';
	*kernel_size = '\0';
	*crc = '\0';
	
	if(!msg || !kernel_name || !addr || !kernel_size || !crc) 
		return -1;

#ifdef WANP
	msg += WAN_HEADER_SIZE;
#endif
	// #else
	*kernel_name = '\0';
	*addr = '\0';
	*kernel_size = '\0';
	ptr = strtok_r((char *)msg, " \r", &save_ptr); // '\r' is the used for WAN message
	while(ptr != NULL) {
		if(strcmp(ptr, "-s") == 0) {
			ptr = strtok_r(NULL, " \r", &save_ptr); // '\r' is the used for WAN message
			if(NULL == ptr) {
				return -1;
			}
			strcpy((char *)kernel_size, ptr);
		} else if(strcmp(ptr, "-h") == 0) {
			return HELP_BURN;
		}else if(strcmp(ptr, "-f") == 0) {
			ptr = strtok_r(NULL, " \r", &save_ptr); // '\r' is the used for WAN message
			if(NULL == ptr) {
				return -1;
			}
			strcpy((char *)kernel_name, ptr);
		} else if(strcmp(ptr, "-a") == 0) {
			ptr = strtok_r(NULL, " \r", &save_ptr); // '\r' is the used for WAN message
			if(NULL == ptr) {
				return -1;
			}
			strcpy((char *)addr, ptr);
		} else if(strcmp(ptr, "-c") == 0) {
			ptr = strtok_r(NULL, " \r", &save_ptr); // '\r' is the used for WAN message
			if(NULL == ptr) {
				strcpy((char *)crc, "0");
			} else {
				strcpy((char *)crc, ptr);
			}
		} else {
			ptr = strtok_r(NULL, " \r", &save_ptr); // '\r' is the used for WAN message
		}
	}
	// #endif
	
	return 0;
}

int DoStartos(uint8_t * msg)
{
	if(!msg) return -1;

#ifdef WANP
	msg += WAN_HEADER_SIZE;
#endif
	// #else
	
	if(strstr((const char *)msg, "-h") != NULL) {
		return HELP_STARTOS;
	}

	// #endif
	return 0;
}

int SealPacket(uint8_t *msg) 
{
	uint16_t msg_size;
#ifdef WANP
	uint16_t msg_size_net;
	uint16_t csum;
	if(!msg) return -1;

	msg_size = strlen(msg + WAN_HEADER_SIZE) + WAN_HEADER_SIZE;
	msg_size_net = Wan_htons(msg_size);
	memcpy(&msg[6], &msg_size_net, sizeof(uint16_t));
	csum = CheckSum(msg, msg_size);
	csum = Wan_htons(csum);
	memcpy(&msg[3], &csum, sizeof(uint16_t));
	if(CheckSum(msg, msg_size) != 0) {
		printf("Warning: set checksum error\n");
		return -1;
	}
#else
	msg_size = strlen(msg);
#endif
	return msg_size;
}

