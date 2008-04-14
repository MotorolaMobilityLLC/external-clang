// RUN: clang -rewrite-objc %s -o=-

@interface MyList
- (id) allKeys;
@end
    
@implementation MyList
- (unsigned int)countByEnumeratingWithState:  (struct __objcFastEnumerationState *)state objects:  (id *)items count:(unsigned int)stackcount
{
        return 0;
}
- (id) allKeys {}
@end

@interface MyList (BasicTest)
- (void)compilerTestAgainst;
@end

int LOOP();
@implementation MyList (BasicTest)
- (void)compilerTestAgainst {
  MyList * el;
        for (el in [el allKeys]) { LOOP(); 
	  }

        for (id el1 in[el allKeys]) { LOOP(); 
	  }
        for (el in([el allKeys])) { LOOP(); 
	  }
}
@end

