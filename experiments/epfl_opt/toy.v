module top(x0, x1, x2, x3, y0);
  input x0, x1, x2, x3, x4, x5;
  output y0;
  wire n0, n1, n2, n3, n4, n5;
  assign n0 = x0 ^ x1;
  assign n1 = x2 ^ x3;
  assign n2 = n1;
  assign n3 = n0 & n2;
  assign n4 = n3 & x4;
  assign n5 = n4 & x5;
  assign y0 = n5;
endmodule
