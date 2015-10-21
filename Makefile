dwmstatus: dwmstatus.c
	gcc -o dwmstatus -Wall -pedantic -std=c99 -lX11 -lm dwmstatus.c

clean:
	$(RM) dwmstatus

.PHONY: lcean
