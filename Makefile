CC = gcc
PREFIX = /usr
OPTS = -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations #-O2 -Werror -Wunused
OBJS = msrp.o msrp_session.o msrp_message.o msrp_relay.o msrp_switch.o msrp_callback.o msrp_network.o msrp_utils.o
APPS = endpoint switch #relay
LIBS = -lpthread #-lssl -lcrypto

so: $(OBJS)
	$(CC) -ggdb -shared -Wl,-soname,libmsrp.so.0 -o libmsrp.so.0.0.2 $(OBJS) $(OPTS) $(LIBS)

%.o: %.c
	$(CC) -fPIC -ggdb -c $< -o $@ $(OPTS)

test: $(APPS)

endpoint: endpointmsrp.o $(OBJS)
	$(CC) -ggdb -o endpointmsrp endpointmsrp.o $(OBJS) $(OPTS) $(LIBS) #-lmsrp

relay: relaymsrp.o
	$(CC) -ggdb -o relaymsrp relaymsrp.o $(OPTS) $(LIBS) -lmsrp

switch: switchmsrp.o $(OBJS)
	$(CC) -ggdb -o switchmsrp switchmsrp.o $(OBJS) $(OPTS) $(LIBS) #-lmsrp

clean:
	rm -f *.o *.obj libmsrp* endpointmsrp relaymsrp switchmsrp

all: clean so test

install:
	@echo Installing MSRP library to $(PREFIX)/lib/:
	install -m 755 libmsrp.so.0.0.2 $(PREFIX)/lib/
	ln -sf $(PREFIX)/lib/libmsrp.so.0.0.2 $(PREFIX)/lib/libmsrp.so.0
	ln -sf $(PREFIX)/lib/libmsrp.so.0 $(PREFIX)/lib/libmsrp.so
	@echo Installing MSRP headers to $(PREFIX)/include/:
	install -m 755 msrp.h $(PREFIX)/include/

uninstall:
	@echo Uninstalling MSRP library from $(PREFIX)/lib/:
	rm -f $(PREFIX)/lib/libmsrp*
	@echo Uninstalling MSRP headers from $(PREFIX)/include/:
	rm -f $(PREFIX)/include/msrp.h
