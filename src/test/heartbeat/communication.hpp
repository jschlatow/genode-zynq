#ifndef COMMUNICATION_HPP__
#define COMMUNICATION_HPP__

#include <curl/curl.h>
#include <json.hpp>

void initCurl(){
	curl_global_init(CURL_GLOBAL_ALL);	
}

void cleanupCurl(){
	curl_global_cleanup();	
}

size_t storeInputData(void* buffer, size_t size, size_t nmemb, void* userdata){
	std::string* result = reinterpret_cast<std::string*>(userdata);
    result->append(static_cast<char*>(buffer), size*nmemb);
    return size*nmemb;
}

std::tuple<int, nlohmann::json> postRequest(const std::string& url, const nlohmann::json& content){

	CURL *curl = curl_easy_init();

	curl_slist* jsonheaders = NULL;

  	jsonheaders = curl_slist_append(jsonheaders, "Accept: application/json");
  	jsonheaders = curl_slist_append(jsonheaders, "Content-Type: application/json");

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, jsonheaders);
		std::string contentStr = content.dump();
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, contentStr.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, storeInputData);
		std::string result;
    	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
//    	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 40);
    	
		CURLcode res = curl_easy_perform(curl);
		curl_slist_free_all(jsonheaders);		

		if(res != CURLE_OK){
			curl_easy_cleanup(curl);
			throw std::runtime_error(curl_easy_strerror(res));
		}
		
		long rCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rCode);  

		curl_easy_cleanup(curl);

		nlohmann::json jResult;
		if(nlohmann::json::accept(result)){
			jResult = nlohmann::json::parse(result);
		}	
		else{
			throw std::runtime_error("response: wrong format: \n" + result 
				+ "Code: \n" + std::to_string(rCode));
		}	
		return {rCode, jResult};
  	}
	else{
		throw std::runtime_error("Curl could not be initialized");
	}	

	
}

std::tuple<int, nlohmann::json> getRequest(const std::string& url){

	CURL *curl = curl_easy_init();

	curl_slist* jsonheaders = NULL;

  	jsonheaders = curl_slist_append(jsonheaders, "Accept: application/json");

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, jsonheaders);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, storeInputData);
		std::string result;
    	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
//    	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 40);

		CURLcode res = curl_easy_perform(curl);		
		curl_slist_free_all(jsonheaders);

		if(res != CURLE_OK){
			curl_easy_cleanup(curl);
			throw std::runtime_error(curl_easy_strerror(res));
		}
		
		long rCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rCode);  
		
		curl_easy_cleanup(curl);

		nlohmann::json jResult;
		if(nlohmann::json::accept(result)){
			jResult = nlohmann::json::parse(result);
		}	
		else{
			throw std::runtime_error("response: wrong format: \n" + result 
				+ "Code: \n" + std::to_string(rCode));
		}	

		return {rCode, jResult};
  	}
	else{
		throw std::runtime_error("Curl could not be initialized");
	}		
}

#endif // !COMMUNICATION_HPP__
