all:
	$(CXX) $(CXXFLAGS) publisher.cpp  -o publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) subscriber.cpp -o subscriber $(LDFLAGS) -lblackadder -lpthread
clean:
	rm -f channel_publisher channel_subscriber publisher subscriber nb_publisher nb_subscriber broadcast_publisher broadcast_subscriber algid_subscriber algid_publisher nb_channel_publisher nb_channel_subscriber
