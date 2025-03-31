// Benchmark "/tmp/tmp" written by ABC on Mon Mar 31 21:10:10 2025

module \/tmp/tmp  ( 
    n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11,
    po0, po1, po2, po3, po4, po5, po6  );
  input  n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11;
  output po0, po1, po2, po3, po4, po5, po6;
  wire new_n12_, new_n13_, new_n14_, new_n15_, new_n16_, new_n17_, new_n18_,
    new_n19_, new_n20_, new_n21_, new_n22_, new_n23_, new_n24_, new_n25_,
    new_n26_, new_n27_, new_n28_, new_n29_, new_n30_, new_n31_, new_n32_,
    new_n33_, new_n34_, new_n35_, new_n36_, new_n37_, new_n38_, new_n39_,
    new_n40_, new_n41_, new_n42_, new_n43_, new_n44_, new_n45_, new_n46_,
    new_n47_, new_n48_, new_n49_, new_n50_, new_n51_, new_n52_, new_n53_,
    new_n54_, new_n55_, new_n56_, new_n57_, new_n58_, new_n59_, new_n60_,
    new_n61_, new_n62_, new_n63_, new_n64_, new_n65_, new_n66_, new_n67_,
    new_n68_, new_n69_, new_n70_, new_n71_, new_n72_, new_n73_, new_n74_,
    new_n75_, new_n76_, new_n77_, new_n78_, new_n79_, new_n80_, new_n81_,
    new_n82_, new_n83_, new_n84_, new_n85_, new_n86_, new_n87_, new_n88_,
    new_n89_, new_n90_, new_n91_, new_n92_, new_n93_, new_n94_, new_n95_,
    new_n96_, new_n97_, new_n98_, new_n99_, new_n100_, new_n101_,
    new_n102_, new_n103_, new_n104_, new_n105_, new_n106_, new_n107_,
    new_n108_, new_n109_, new_n110_, new_n111_, new_n112_, new_n113_,
    new_n114_, new_n115_, new_n116_, new_n117_, new_n118_, new_n119_,
    new_n120_, new_n121_, new_n122_, new_n123_, new_n124_, new_n125_,
    new_n126_, new_n127_, new_n128_, new_n129_, new_n130_, new_n131_,
    new_n132_, new_n133_, new_n134_, new_n135_, new_n136_, new_n137_,
    new_n138_, new_n139_, new_n140_, new_n141_, new_n142_, new_n143_,
    new_n144_, new_n145_, new_n146_, new_n147_, new_n148_, new_n149_,
    new_n150_, new_n151_, new_n152_, new_n153_, new_n154_, new_n155_,
    new_n156_, new_n157_, new_n158_, new_n159_, new_n160_, new_n161_,
    new_n162_, new_n163_, new_n164_, new_n165_, new_n166_, new_n167_,
    new_n168_, new_n169_, new_n170_, new_n171_, new_n172_, new_n173_,
    new_n174_, new_n175_, new_n176_, new_n177_, new_n178_, new_n179_,
    new_n180_, new_n181_, new_n182_, new_n183_, new_n184_, new_n185_,
    new_n186_, new_n187_, new_n188_, new_n189_, new_n190_, new_n191_,
    new_n192_, new_n193_, new_n194_, new_n195_, new_n196_, new_n197_,
    new_n198_, new_n199_, new_n200_, new_n201_, new_n202_, new_n203_,
    new_n204_, new_n205_, new_n206_, new_n207_, new_n208_, new_n209_,
    new_n210_, new_n211_, new_n212_, new_n213_, new_n214_, new_n215_,
    new_n216_, new_n217_, new_n218_, new_n219_, new_n220_, new_n221_,
    new_n222_, new_n223_, new_n224_, new_n225_, new_n226_, new_n227_,
    new_n228_, new_n229_, new_n230_;
  assign new_n12_ = ~n5 & ~n9;
  assign new_n13_ = new_n12_ ^ n5;
  assign new_n14_ = new_n13_ ^ n9;
  assign new_n15_ = ~n8 & n1;
  assign new_n16_ = ~n7 & new_n13_;
  assign new_n17_ = new_n15_ & new_n16_;
  assign new_n18_ = ~n5 & n8;
  assign new_n19_ = n8 ^ n5;
  assign new_n20_ = new_n19_ ^ new_n18_;
  assign new_n21_ = n2 & new_n20_;
  assign new_n22_ = ~n7 & new_n21_;
  assign new_n23_ = new_n22_ ^ new_n17_;
  assign new_n24_ = ~new_n23_ & new_n14_;
  assign new_n25_ = ~n6 & ~new_n24_;
  assign new_n26_ = ~n9 & ~new_n18_;
  assign new_n27_ = n5 & n6;
  assign new_n28_ = new_n27_ ^ n6;
  assign new_n29_ = n9 & new_n28_;
  assign new_n30_ = new_n29_ ^ new_n26_;
  assign new_n31_ = n4 & n9;
  assign new_n32_ = new_n31_ ^ n4;
  assign new_n33_ = ~n8 & ~n9;
  assign new_n34_ = ~new_n32_ & ~new_n33_;
  assign new_n35_ = new_n34_ ^ new_n30_;
  assign new_n36_ = ~n3 & ~n8;
  assign new_n37_ = n2 & new_n36_;
  assign new_n38_ = n6 & new_n37_;
  assign new_n39_ = new_n32_ & new_n38_;
  assign new_n40_ = new_n39_ ^ new_n35_;
  assign new_n41_ = ~new_n25_ & new_n40_;
  assign new_n42_ = ~n10 & ~new_n41_;
  assign new_n43_ = ~n6 & ~n7;
  assign new_n44_ = new_n43_ ^ n6;
  assign new_n45_ = ~new_n44_ & n10;
  assign new_n46_ = n2 & n3;
  assign new_n47_ = new_n46_ ^ n2;
  assign new_n48_ = ~new_n18_ & new_n47_;
  assign new_n49_ = ~n4 & n5;
  assign new_n50_ = new_n49_ ^ n5;
  assign new_n51_ = ~new_n50_ & new_n14_;
  assign new_n52_ = new_n48_ & new_n51_;
  assign new_n53_ = new_n46_ ^ n3;
  assign new_n54_ = new_n33_ & new_n53_;
  assign new_n55_ = ~n10 & ~new_n54_;
  assign new_n56_ = ~new_n52_ & new_n55_;
  assign new_n57_ = new_n44_ ^ n7;
  assign new_n58_ = new_n57_ ^ n6;
  assign new_n59_ = ~new_n56_ & ~new_n58_;
  assign new_n60_ = ~new_n45_ & ~new_n59_;
  assign new_n61_ = ~new_n42_ & new_n60_;
  assign new_n62_ = ~n11 & ~new_n61_;
  assign new_n63_ = ~n8 & n7;
  assign new_n64_ = new_n63_ ^ n7;
  assign new_n65_ = new_n64_ ^ n8;
  assign new_n66_ = n11 & new_n65_;
  assign new_n67_ = n4 ^ n3;
  assign new_n68_ = ~n10 & n9;
  assign new_n69_ = new_n68_ ^ n10;
  assign new_n70_ = ~new_n69_ & new_n67_;
  assign new_n71_ = ~n11 & ~new_n70_;
  assign new_n72_ = ~n8 & ~new_n71_;
  assign new_n73_ = n9 & n11;
  assign new_n74_ = n10 & new_n73_;
  assign new_n75_ = ~new_n72_ & ~new_n74_;
  assign new_n76_ = ~new_n75_ & n7;
  assign new_n77_ = ~new_n66_ & ~new_n76_;
  assign new_n78_ = ~new_n62_ & new_n77_;
  assign new_n79_ = ~n1 & n3;
  assign new_n80_ = ~new_n79_ & new_n20_;
  assign new_n81_ = n1 & n2;
  assign new_n82_ = n3 & new_n81_;
  assign new_n83_ = new_n82_ ^ n1;
  assign new_n84_ = ~new_n83_ & new_n80_;
  assign new_n85_ = ~n5 & ~n10;
  assign new_n86_ = ~new_n36_ & ~new_n85_;
  assign new_n87_ = ~n2 & ~new_n86_;
  assign new_n88_ = ~new_n68_ & ~new_n87_;
  assign new_n89_ = ~new_n84_ & new_n88_;
  assign new_n90_ = ~n7 & ~new_n89_;
  assign new_n91_ = ~new_n69_ & n8;
  assign new_n92_ = ~new_n50_ & new_n91_;
  assign new_n93_ = ~n8 & n10;
  assign new_n94_ = new_n93_ ^ new_n92_;
  assign new_n95_ = ~new_n90_ & ~new_n94_;
  assign new_n96_ = ~n6 & ~new_n95_;
  assign new_n97_ = ~n5 & ~n7;
  assign new_n98_ = ~n8 & ~new_n46_;
  assign new_n99_ = new_n98_ ^ n8;
  assign new_n100_ = ~new_n99_ & new_n97_;
  assign new_n101_ = ~n10 & ~new_n13_;
  assign new_n102_ = new_n85_ ^ new_n69_;
  assign new_n103_ = new_n102_ ^ new_n101_;
  assign new_n104_ = ~new_n98_ & ~new_n103_;
  assign new_n105_ = ~new_n100_ & ~new_n104_;
  assign new_n106_ = ~new_n105_ & n4;
  assign new_n107_ = n7 & new_n69_;
  assign new_n108_ = new_n93_ ^ new_n85_;
  assign new_n109_ = ~new_n108_ & new_n107_;
  assign new_n110_ = ~new_n106_ & ~new_n109_;
  assign new_n111_ = ~new_n110_ & n6;
  assign new_n112_ = new_n101_ ^ new_n93_;
  assign new_n113_ = ~n7 & new_n112_;
  assign new_n114_ = ~new_n111_ & ~new_n113_;
  assign new_n115_ = ~new_n96_ & new_n114_;
  assign new_n116_ = ~n11 & ~new_n115_;
  assign new_n117_ = ~n10 & new_n64_;
  assign new_n118_ = new_n73_ & new_n117_;
  assign new_n119_ = ~n4 & ~new_n58_;
  assign new_n120_ = new_n46_ & new_n119_;
  assign new_n121_ = new_n120_ ^ new_n119_;
  assign new_n122_ = ~n10 & n7;
  assign new_n123_ = ~n3 & new_n122_;
  assign new_n124_ = n5 & new_n122_;
  assign new_n125_ = ~n4 & n3;
  assign new_n126_ = new_n122_ & new_n125_;
  assign new_n127_ = new_n126_ ^ new_n124_;
  assign new_n128_ = new_n127_ ^ new_n123_;
  assign new_n129_ = new_n128_ ^ new_n121_;
  assign new_n130_ = ~n8 & new_n129_;
  assign new_n131_ = ~n11 & ~new_n130_;
  assign new_n132_ = ~n9 & ~new_n64_;
  assign new_n133_ = ~new_n131_ & new_n132_;
  assign new_n134_ = ~new_n118_ & ~new_n133_;
  assign new_n135_ = ~new_n116_ & new_n134_;
  assign new_n136_ = n4 & new_n28_;
  assign new_n137_ = n1 & new_n49_;
  assign new_n138_ = ~n7 & new_n137_;
  assign new_n139_ = new_n138_ ^ new_n136_;
  assign new_n140_ = n2 & new_n139_;
  assign new_n141_ = ~new_n81_ & new_n50_;
  assign new_n142_ = new_n141_ ^ new_n97_;
  assign new_n143_ = ~n6 & new_n142_;
  assign new_n144_ = ~new_n140_ & ~new_n143_;
  assign new_n145_ = ~new_n144_ & n3;
  assign new_n146_ = n3 & n7;
  assign new_n147_ = n7 ^ n3;
  assign new_n148_ = new_n147_ ^ new_n146_;
  assign new_n149_ = ~new_n148_ & n4;
  assign new_n150_ = ~n4 & n6;
  assign new_n151_ = new_n150_ ^ new_n149_;
  assign new_n152_ = n5 & new_n151_;
  assign new_n153_ = ~new_n145_ & ~new_n152_;
  assign new_n154_ = ~n8 & ~new_n153_;
  assign new_n155_ = ~new_n50_ & ~new_n57_;
  assign new_n156_ = ~n2 & ~new_n58_;
  assign new_n157_ = ~n6 & new_n146_;
  assign new_n158_ = new_n157_ ^ new_n156_;
  assign new_n159_ = new_n50_ & new_n158_;
  assign new_n160_ = ~new_n155_ & ~new_n159_;
  assign new_n161_ = ~new_n154_ & new_n160_;
  assign new_n162_ = ~n9 & ~new_n161_;
  assign new_n163_ = ~new_n27_ & new_n64_;
  assign new_n164_ = ~n3 & new_n63_;
  assign new_n165_ = n4 & new_n65_;
  assign new_n166_ = new_n165_ ^ new_n164_;
  assign new_n167_ = new_n27_ & new_n166_;
  assign new_n168_ = ~new_n163_ & ~new_n167_;
  assign new_n169_ = ~new_n162_ & new_n168_;
  assign new_n170_ = ~n10 & ~new_n169_;
  assign new_n171_ = new_n27_ & new_n63_;
  assign new_n172_ = new_n171_ ^ new_n65_;
  assign new_n173_ = n9 & new_n172_;
  assign new_n174_ = ~new_n170_ & ~new_n173_;
  assign new_n175_ = ~n11 & ~new_n174_;
  assign new_n176_ = n6 & n10;
  assign new_n177_ = ~n9 & new_n176_;
  assign new_n178_ = new_n177_ ^ new_n73_;
  assign new_n179_ = new_n64_ & new_n178_;
  assign new_n180_ = ~n6 & ~n8;
  assign new_n181_ = n8 ^ n6;
  assign new_n182_ = new_n181_ ^ new_n180_;
  assign new_n183_ = n9 & new_n182_;
  assign new_n184_ = ~n11 & ~new_n183_;
  assign new_n185_ = ~new_n184_ & n10;
  assign new_n186_ = ~new_n179_ & ~new_n185_;
  assign new_n187_ = ~new_n175_ & new_n186_;
  assign new_n188_ = ~n10 & ~n11;
  assign new_n189_ = ~n4 & new_n188_;
  assign new_n190_ = ~n8 & new_n43_;
  assign new_n191_ = new_n12_ & new_n190_;
  assign new_n192_ = ~new_n57_ & n8;
  assign new_n193_ = n3 & new_n192_;
  assign new_n194_ = new_n193_ ^ new_n192_;
  assign new_n195_ = ~new_n14_ & new_n194_;
  assign new_n196_ = new_n195_ ^ new_n191_;
  assign new_n197_ = new_n189_ & new_n196_;
  assign new_n198_ = new_n57_ & new_n99_;
  assign new_n199_ = ~new_n198_ & n4;
  assign new_n200_ = ~new_n180_ & ~new_n199_;
  assign new_n201_ = ~new_n200_ & n5;
  assign new_n202_ = ~new_n63_ & ~new_n201_;
  assign new_n203_ = new_n43_ & new_n81_;
  assign new_n204_ = ~new_n171_ & ~new_n203_;
  assign new_n205_ = new_n125_ ^ n3;
  assign new_n206_ = ~new_n204_ & new_n205_;
  assign new_n207_ = ~new_n202_ & ~new_n206_;
  assign new_n208_ = ~n9 & ~new_n207_;
  assign new_n209_ = n9 & new_n192_;
  assign new_n210_ = ~new_n209_ & n10;
  assign new_n211_ = new_n27_ & new_n117_;
  assign new_n212_ = new_n125_ ^ new_n31_;
  assign new_n213_ = new_n211_ & new_n212_;
  assign new_n214_ = ~new_n210_ & ~new_n213_;
  assign new_n215_ = ~new_n208_ & new_n214_;
  assign new_n216_ = ~n11 & ~new_n215_;
  assign new_n217_ = new_n31_ & new_n194_;
  assign new_n218_ = new_n32_ & new_n180_;
  assign new_n219_ = new_n82_ & new_n218_;
  assign new_n220_ = n9 & new_n193_;
  assign new_n221_ = new_n220_ ^ new_n219_;
  assign new_n222_ = new_n221_ ^ new_n217_;
  assign new_n223_ = n5 & new_n222_;
  assign new_n224_ = n3 & new_n50_;
  assign new_n225_ = ~new_n57_ & new_n224_;
  assign new_n226_ = new_n33_ & new_n188_;
  assign new_n227_ = ~new_n225_ & new_n226_;
  assign new_n228_ = ~new_n43_ & new_n227_;
  assign new_n229_ = ~new_n228_ & new_n188_;
  assign new_n230_ = ~new_n223_ & new_n229_;
  assign po0 = ~new_n78_;
  assign po1 = new_n135_;
  assign po2 = ~new_n187_;
  assign po3 = ~new_n197_;
  assign po4 = ~new_n216_;
  assign po5 = ~new_n230_;
  assign po6 = ~new_n227_;
endmodule


