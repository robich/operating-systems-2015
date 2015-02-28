EXECUTABLES = uniq_sample_test uniq_concurr_test child_pids_sample_test child_pids_concurr_test

.PHONY: build
build:	$(EXECUTABLES)

uniq_sample_test: uniq_sample_test.c syscalls.h
	gcc uniq_sample_test.c -o uniq_sample_test
	
uniq_concurr_test: uniq_concurr_test.c syscalls.h
	gcc uniq_concurr_test.c -lpthread -o uniq_concurr_test

child_pids_sample_test: child_pids_sample_test.c syscalls.h
	gcc child_pids_sample_test.c -o child_pids_sample_test

child_pids_concurr_test: child_pids_concurr_test.c syscalls.h
	gcc child_pids_concurr_test.c -lpthread -o child_pids_concurr_test

.PHONY: clean
clean:
	-rm $(EXECUTABLES)
