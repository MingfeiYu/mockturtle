module top( x0 , x1 , x2 , x3 , x4 , x5 , x6 , x7 , y0 );
  input x0 , x1 , x2 , x3 , x4 , x5 , x6 , x7 ;
  output y0 ;
  wire n9 , n10 , n11 , n12 , n13 , n14 , n15 , n16 , n17 , n18 , n19 , n20 , n21 , n22 , n23 , n24 , n25 , n26 , n27 , n28 , n29 , n30 , n31 , n32 , n33 , n34 , n35 , n36 , n37 , n38 ;
  assign n11 = x6 ^ x5 ;
  assign n24 = x7 ^ x6 ;
  assign n25 = n11 & ~n24 ;
  assign n26 = n25 ^ x5 ;
  assign n9 = x3 ^ x2 ;
  assign n20 = x4 ^ x3 ;
  assign n21 = n9 & ~n20 ;
  assign n22 = n21 ^ x2 ;
  assign n27 = n26 ^ n22 ;
  assign n12 = n11 ^ x7 ;
  assign n13 = n12 ^ x1 ;
  assign n10 = n9 ^ x4 ;
  assign n16 = n12 ^ n10 ;
  assign n17 = n13 & ~n16 ;
  assign n18 = n17 ^ x1 ;
  assign n14 = n13 ^ n10 ;
  assign n15 = x0 & n14 ;
  assign n19 = n18 ^ n15 ;
  assign n23 = n22 ^ n19 ;
  assign n28 = n27 ^ n23 ;
  assign n29 = n26 ^ n18 ;
  assign n30 = n29 ^ n22 ;
  assign n31 = ~n19 & n29 ;
  assign n32 = n31 ^ n19 ;
  assign n33 = n30 & ~n32 ;
  assign n34 = n33 ^ n22 ;
  assign n35 = n28 & n34 ;
  assign n36 = n35 ^ n31 ;
  assign n37 = n36 ^ n22 ;
  assign n38 = n37 ^ n27 ;
  assign y0 = n38 ;
endmodule