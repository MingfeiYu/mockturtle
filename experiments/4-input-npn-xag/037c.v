module top( x0 , x1 , x2 , x3 , y0 );
  input x0 , x1 , x2 , x3 ;
  output y0 ;
  wire n5 , n6 , n7 , n8 , n9 ;
  assign n5 = x3 ^ x1 ;
  assign n6 = x0 & n5 ;
  assign n7 = x1 & ~n6 ;
  assign n8 = x2 & ~n7 ;
  assign n9 = n8 ^ n5 ;
  assign y0 = n9 ;
endmodule
