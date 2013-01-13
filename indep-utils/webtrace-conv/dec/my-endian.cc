#include "my-endian.h"
 
/* rotates 2 bytes */
unsigned short swap2(u_2bytes In) {
	u_2bytes Out;

	((char*)&(Out))[0] = ((char*)&(In))[1];
	((char*)&(Out))[1] = ((char*)&(In))[0];

	return Out;
}

/* rotates 4 bytes */
u_4bytes rotate4(u_4bytes In) {
	u_4bytes Out;

	((u_2bytes*)&Out)[0] = swap2(((u_2bytes*)&In)[1]);
	((u_2bytes*)&Out)[1] = swap2(((u_2bytes*)&In)[0]);

	return Out;
}

/* detects endian-ness
 * Note:   will not work if sizeof(unsigned long)==1
 */
int IsLittleEndian(void) {
	static const unsigned long long_number = 1;
	return *(const unsigned char *)&long_number;
}

/* changes endian-ness */
void ToOtherEndian(TEntry *e) {

	/* unroll this loop if you want to enumerate u_4bytes members of TEntry_v2 */
	u_4bytes *p;
	for (p = &(e -> head.event_duration); p <= &(e -> url); p++)
		*p = rotate4(*p);

	e -> tail.status = swap2(e -> tail.status);
	
	if (sizeof(method_t) == 2)
		e -> tail.method = swap2(e -> tail.method);
	else
	if (sizeof(method_t) == 4)
		e -> tail.method = rotate4(e -> tail.method);

	if (sizeof(protocol_t) == 2)
		e -> tail.protocol = swap2(e -> tail.protocol);
	else
	if (sizeof(protocol_t) == 4)
		e -> tail.protocol = rotate4(e -> tail.protocol);
}
