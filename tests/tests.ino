struct Bar {
  Bar(const char* s)
    : str(s) {}
  ~Bar() {
    Serial.print(str);
    Serial.println(" destroyed");
  }
  const char* str;
};

void setup() {
  auto const& str1 = String("hello") + "!";
  auto const str2 = String("world");
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println(str1);
  Serial.println(str2);

  auto const& bar1 = Bar("hello");
  Serial.println("----");
  auto const bar2 = Bar("world");
}

void loop() {
}
