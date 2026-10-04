#include <iostream>
#include <bits/stdc++.h>

using namespace std;

// Product sold by the machine: just a name and a mutable price.
class Item {
private:
	string name;
	int price;

public:
	Item(string name,int price) {
		this->name=name;
		this->price=price;
	}

	int getPrice() {
		return price;
	}

	void updatePrice(int price) {
		this->price=price;
	}

	string getName() {
		return name;
	}



};

// A physical slot in the machine holding one Item and a stock count.
class Slot {
private:
	string id;
	Item *item;
	int quantity;

public:
	Slot(string id, Item& item, int quantity) {
		this->id=id;
		this->item=&item;
		this->quantity=quantity;
	}

	bool isAvaliable() {
		return quantity>0;
	}

	Item* getItem() {
		return item;
	}

	void ReduceQuantity() {
		quantity=quantity-1;
		cout<<"Quantity reduced"<<endl;

	}
};



// Strategy interface for payment methods. Each concrete gateway decides
// whether a payment of `pay` covers `price` and reports any change owed.
class PaymentGateway {
public:
	struct PaymentResult {
		bool success;
		int changeAmount;
	};
	virtual PaymentResult pay(int pay,int price)=0;
	virtual ~PaymentGateway()=default;
};

// UPI/card payments are exact-amount only — no change is ever handed back.
class UPI:public PaymentGateway {
	PaymentResult pay(int Amountpaid,int AmountToBepaid) override {
		return {Amountpaid>=AmountToBepaid,0};
	}
};

class Card:public PaymentGateway {
	PaymentResult pay(int Amountpaid,int AmountToBepaid) override {
		return {Amountpaid>=AmountToBepaid,0};
	}
};

// Cash payments can overpay; the machine must compute and dispense change.
class Cash:public PaymentGateway {
	PaymentResult pay(int Amountpaid,int AmountToBepaid) override {
		int change=Amountpaid-AmountToBepaid;
		if(change>=0) return {true,change};
		else {
			cout<<"amount paid is less then price"<<endl;
			return {false,0};
		}
	}
};

// Tracks how many notes of each denomination the machine has on hand and
// works out a greedy breakdown to return as change.
class CashInventory {

private:
	map<int,int> cashInventory;             // denomination -> note count (map keeps keys sorted ascending)
	vector<pair<int,int>> changeDenominations; // last computed breakdown: {denomination, count}
public:
	// Seed the till with a starting stock of each note.
	CashInventory() {
		cashInventory.emplace(100,0);
		cashInventory.emplace(50,1);
		cashInventory.emplace(20,0);
		cashInventory.emplace(10,10);
		cashInventory.emplace(1,100);

		changeDenominations= {};

	}

	// Greedily picks largest-denomination notes first to cover `reqchange`.
	// On success, fills changeDenominations and returns true; on failure
	// (not enough notes to make exact change) clears it and returns false.
	bool canMakeChange(int reqchange) {
		changeDenominations= {};

		bool Possible=true;
		auto it=prev(cashInventory.end()); // start from the highest denomination

		while(it!=cashInventory.begin() && reqchange!=0) {
			if(reqchange / it->first > 0) {
				int notes=reqchange / it->first;
				if(notes > it->second) {
					notes=it->second; // can't use more notes than are in stock
				}
				changeDenominations.push_back({it->first,notes});
				reqchange=reqchange - (it->first*notes);
			}
			it--;
		}

		if (reqchange != 0) {
			changeDenominations.clear();
			return false;
		}
		return reqchange==0;


	}


	// Deducts the previously computed change breakdown from stock and
	// returns it so the caller can report/hand out the notes.
	vector<pair<int,int>> despenseChange() {
		for(int i=0; i<changeDenominations.size(); i++) {
			auto it=cashInventory.find(changeDenominations[i].first);
			if(it!=cashInventory.end()) {
				it->second=it->second-changeDenominations[i].second;
			}
		}
		return changeDenominations;
	}
};


// Orchestrates slot selection, payment, and dispensing.
class VendingMachine {
private:
	unordered_map<string,Slot*> slots;
	Slot*selectedSlot=nullptr;
public:
	VendingMachine( unordered_map<string,Slot*>& slots) {
		this->slots=slots;
	}

	// Validates the slot id and stock, then remembers it for MakePayment.
	void selectSlot(string slotId) {
		auto it=slots.find(slotId);

		if(it==slots.end()) {
			std::cout << "slot is not avaliable" << std::endl;
			return;
		}

		if(it->second->isAvaliable()) {
			selectedSlot=it->second;
			cout<<"slot is avaliable and product is "<<selectedSlot->getItem()->getName()<<endl;
		}
		else {
			cout<<"product is not avaliable" << endl;
		}

		return;

	}

	// Decrements stock in the currently selected slot.
	void dispenseProduct() {
		cout<<"dispencing product"<<endl;
		selectedSlot->ReduceQuantity();
		cout<<"Product dispensed"<<endl;
	}


	// Runs the selected gateway's pay() against the selected item's price,
	// then dispenses the product and/or change depending on the result.
	void MakePayment(int amount,PaymentGateway& type, CashInventory& cashInventory) {
		if (selectedSlot == nullptr) {
			cout << "select item first" << endl;
			return;
		}

		Item* it=selectedSlot->getItem();
		int price=it->getPrice();
		cout<<"price="<<price<<endl;
		auto r=type.pay(amount,price);
		if(r.success && r.changeAmount==0) {
			// Exact payment: dispense immediately, nothing owed back.
			std::cout << "transaction Completed" << std::endl;
			dispenseProduct();
		}
		else if(r.success && r.changeAmount>0) {
			// Overpaid: try to break the change into available notes first.
			cout<<"cashing  and handing out  the change"<<endl;
			bool changeAvaliable=cashInventory.canMakeChange(r.changeAmount);
			if(changeAvaliable) {
				vector<pair<int,int>> changeBills=cashInventory.despenseChange();
				cout << "change bills:" << endl;
				for (const auto& [note, count] : changeBills) {
					cout << note << " x " << count << endl;
				}
				dispenseProduct();
			}
			else {
				// Don't dispense if we can't give correct change back.
				cout<<"sufficient change is not avaliable try other payment methods"<<endl;
			}
		}
		else {
			std::cout << "transaction failed" << std::endl;
		}
	}
};


// Demo: stock three slots, select one, and pay cash for it.
int main()
{
	Item it("choco bar",20);
	Item coke("coke",70);
	Item bang("bang",12);
	Slot  A1("A1",it,9);
	Slot  B1("B1",coke,9);
	Slot  C1("C1",bang,9);
	unordered_map<string, Slot*> slots;
	slots.emplace("A1",&A1);
	slots.emplace("B1",&B1);
	slots.emplace("C1",&C1);
	VendingMachine vm(slots);
	vm.selectSlot("B1");
	Card card;
	Cash cash;
	UPI upi;
	CashInventory cashInventory;
	vm.MakePayment(100000,cash,cashInventory);
	return 0;
}