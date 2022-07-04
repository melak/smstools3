# In case of windows, modification is not necessary anymore.

# Uncomment for Solaris
# CFLAGS = -D SOLARIS

# Comment this out, to enable statistics
# This depends on libmm library. If your system does not have it,
# then compile and install it from the directory ../libmm-1.4.0.
CFLAGS += -D NOSTATS

# Uncomment to add debug info
# CFLAGS += -ggdb -O0

# The following options are only used by developers
# CFLAGS += -Wall 
# CFLAGS += -D DEBUGMSG

all: smsd 

smsd: smsd.c extras.o locking.o cfgfile.o logging.o alarm.o smsd_cfg.o charset.o stats.o blacklist.o whitelist.o modeminit.o pdu.o
ifneq (,$(findstring NOSTATS,$(CFLAGS)))
	  $(CC) $(CFLAGS) -o $@ $^
else 
	  $(CC) `mm-config --cflags` $(CFLAGS) -o $@ $^ `mm-config --ldflags --libs`
endif


clean:
	rm -f *.o smsd *.exe
