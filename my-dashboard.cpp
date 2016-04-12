#include <cstdlib>
#include <string>
#include <iostream>
#include <regex>
#include <vector>
#include <ctime>
#include <curl/curl.h>
#include <unistd.h>
#include <fstream>

using namespace std;

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

string slurp(string url) {
  string r;
  CURL *curl_handle;
  CURLcode res;

  struct MemoryStruct chunk;

  chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  /* get it! */
  res = curl_easy_perform(curl_handle);

  /* check for errors */
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
  }
  else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */
    r = chunk.memory;
  }
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  free(chunk.memory);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();
  return r;
}

string re_group(regex re, string text, size_t group) {
  smatch sm;
  regex_search(text, sm, re);
  return sm[group];
}

vector<string> re_groups(regex re, string text, size_t group) {
  regex_iterator<string::iterator> rit(text.begin(), text.end(), re);
  static regex_iterator<string::iterator> rend;

  vector<string> r;

  while (rit != rend) {
    r.push_back((*rit)[group]);
    ++rit;
  }
  return r;
}

// followed by newline
string now() {
  time_t rawtime;

  time(&rawtime);
  return string(ctime(&rawtime));
}

template <typename T>
std::ostream & operator <<(std::ostream & os, const vector<T> & lst) {
  os << "(";
  for (auto itr = lst.begin(); itr != lst.end(); itr++) {
    if (itr != lst.begin()) os << ", ";
    os << *itr;
  }
  os << ")";
  return os;
}

vector<string> list_naver() {
  static regex re("<option value=\".+\">.+: (.+)</option>");
  return re_groups(re, slurp("http://www.naver.com"), 1);
}

vector<string> list_daum() {
  static regex re("<span class=\"txt_issue\">\n.+\n(<.+>)?(.+?)(<.+>)?\n");
  auto r = re_groups(re, slurp("http://www.daum.net"), 2);
  r.erase(unique(r.begin(), r.end()), r.end());
  return r;
}

string kospi() {
  smatch sm;
  static regex re("KOSPI.*?>(.+?)&nbsp;&nbsp;");
  auto text = slurp("http://kosdb.koscom.co.kr/main/jisuticker.html");
  return re_group(re, text, 1);
}

string fx_rate(string code) {
  static regex re(R"__(<option value=\"([\d\.]+)\" label=\"1\" class=\"selectbox-default\" selected=\"selected\" >)__");
  auto text = slurp("http://info.finance.naver.com/marketindex/exchangeDetail.nhn?marketindexCd=FX_" + code);
  return re_group(re, text, 1);
}

string stock(string code) {
  static regex re(R"__(<dd>.+? ([\d\.,]+) .+? .+?)__");
  auto text = slurp("http://finance.naver.com/item/main.nhn?code=" + code);
  return re_group(re, text, 1);
}


int main() {
  const string fx_rate_items[] = { "USDKRW", "EURKRW" };
  while (true) {
    cout << now();
    cout << "Naver: " << list_naver() << "\n";
    cout << "Daum: " << list_daum() << "\n";
    cout << "KOSPI: " << kospi() << "\n";
    for (auto &item : fx_rate_items) {
      cout << item << ": " << fx_rate(item) << "\n";
    }
    
    std::ifstream ifs("my-dashboard.ini");
    if (!ifs) {
      std::cout << "error opening file\n";
      return 1;
    }
    vector<string> items;
    while (!ifs.eof()) {
      string s;
      ifs >> s;
      if (s.length() > 0) items.push_back(s);
    }
    for (auto &item : items) {
      cout << item << ": " << stock(item) << "\n";
    }
    cout << "\n";
    sleep(60);
  }
	
  return 0;
}

