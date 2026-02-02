// Minimal repro for stack-local array indexing.
int main(void) {
  char buf[] = "abcdefgh";
  asm volatile("" : : "r"(buf) : "memory");
  return (unsigned char)buf[2];
}
