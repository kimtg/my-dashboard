my-dashboard: my-dashboard.cpp
	g++ -std=c++11 -s my-dashboard.cpp -o my-dashboard -lcurl
clean:
	rm -rf my-dashboard
run: my-dashboard
	./my-dashboard
