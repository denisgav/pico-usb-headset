class sample_rate_convertion:
	def __init__(self, prod_sample_rate, cons_sample_rate):
		self.prod_stride = int(7056000/prod_sample_rate)
		self.cons_stride = int(7056000/cons_sample_rate)

		self.prod_ptr = 0
		self.cons_ptr = 0

		self.SRC_MAX_BUF_LEN = 128

	def m_print(self):
		return f"prod_ptr={self.prod_ptr}, cons_ptr={self.cons_ptr}"

	def get_next_src_replications(self):
		if(self.prod_stride == self.cons_stride):
			return 1;
		else:
			res = 0;

			self.prod_ptr += self.prod_stride;

			while(self.prod_ptr > self.cons_ptr):
				res += 1
				self.cons_ptr += self.cons_stride;

			if(self.prod_ptr == self.cons_ptr):
				self.prod_ptr = 0
				self.cons_ptr = 0
			else:
				if(self.prod_ptr > self.cons_ptr):
					self.prod_ptr = self.prod_ptr - self.cons_ptr
					self.cons_ptr = 0
				else:
					self.cons_ptr = self.cons_ptr - self.prod_ptr
					self.prod_ptr = 0
		return res;



def main():
	src = sample_rate_convertion(44100, 48000)
	src.prod_stride = int(44*45*48/44)
	src.cons_stride = int(44*45*48/48)

	replications_sum = 0
	for i in range(44):
		replications = src.get_next_src_replications()
		print(f"#{i}, replications = {replications}. {src.m_print()}")
		replications_sum += replications

	print(f"replications_sum = {replications_sum}")

	src.prod_stride = int(44*45*48/45)
	src.cons_stride = int(44*45*48/48)

	replications_sum = 0
	for i in range(45):
		replications = src.get_next_src_replications()
		print(f"#{i}, replications = {replications}. {src.m_print()}")
		replications_sum += replications

	print(f"replications_sum = {replications_sum}")


if __name__ == "__main__":
    main()
