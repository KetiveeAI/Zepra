// Classes

class Animal {
    constructor(name, legs) {
        this.name = name;
        this.legs = legs;
    }

    speak() {
        return this.name + ' makes a sound';
    }

    legCount() {
        return this.legs;
    }
}

var dog = new Animal('Dog', 4);
var dogName = dog.name;
var dogLegs = dog.legCount();
var dogSpeak = dog.speak();

// Inheritance
class Dog extends Animal {
    constructor(name) {
        super(name, 4);
        this.type = 'canine';
    }

    speak() {
        return this.name + ' barks';
    }
}

var rex = new Dog('Rex');
var rexName = rex.name;
var rexLegs = rex.legs;
var rexSpeak = rex.speak();
var rexType = rex.type;

// instanceof
var isDog = (rex instanceof Dog);
var isAnimal = (rex instanceof Animal);

__result = (dogName === 'Dog' && dogLegs === 4 && rexName === 'Rex' && rexLegs === 4 && rexSpeak === 'Rex barks' && rexType === 'canine' && isDog === true && isAnimal === true) ? 1 : 0;
