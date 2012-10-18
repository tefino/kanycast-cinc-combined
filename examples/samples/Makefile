all:
	$(CXX) $(CXXFLAGS) publisher.cpp  -o kc_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) subscriber.cpp -o kc_subscriber $(LDFLAGS) -lblackadder -lpthread
clean:
	rm -f channel_publisher channel_subscriber kc_publisher kc_subscriber nb_publisher nb_subscriber broadcast_publisher broadcast_subscriber algid_subscriber algid_publisher nb_channel_publisher nb_channel_subscriber
