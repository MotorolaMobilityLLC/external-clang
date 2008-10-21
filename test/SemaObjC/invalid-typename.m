// RUN: clang -fsyntax-only -verify %s

@class NSString, NSArray;

@protocol ISyncSessionCallback 
- (oneway void)clientWithId:(bycopy NSString *)clientId
                   canBeginSyncingPlanWithId:(bycopy NSString *)planId
                   syncModes:(bycopy NSArray /* ISDSyncState */ *)syncModes
                   entities:(bycopy NSArray /* ISDEntity */ *)entities
                   truthPullers:(bycopy NSDictionary /* NSString -> [NSString] */ *)truthPullers; // expected-error{{expected ')'}} expected-error{{to match this '('}}
@end

