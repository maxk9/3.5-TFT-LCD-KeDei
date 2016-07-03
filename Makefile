all:
	g++ -O3 lcd_test.cpp -o test
	@echo "done"
	
#clean:
#	rm -rf *.o test $(BIN)
	
clean:
	@echo "Clean"
	@rm -f *.o *~ $(BINS)