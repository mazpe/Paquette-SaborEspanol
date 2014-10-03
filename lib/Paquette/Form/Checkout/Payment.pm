package Paquette::Form::Checkout::Payment;
use HTML::FormHandler::Moose;
use HTML::FormHandler::Types ( 'NoSpaces', 'WordChars' );
extends 'HTML::FormHandler::Model::DBIC';
use Data::Dumper;

with 'HTML::FormHandler::Render::Simple'; # if you want to render the form

before 'setup_form' => sub {
    my $self = shift;

    if ( $self->params->{payment_type} eq "CC" ) {

        $self->field('paypay_email')->inactive(1);

        $self->field('credit_card_number')->inactive(0);
        $self->field('credit_card_exp_month')->inactive(0);
        $self->field('credit_card_exp_year')->inactive(0);
        $self->field('credit_card_cvv')->inactive(0);


    } elsif ( $self->params->{payment_type} eq "PayPal" ) {

        $self->field('paypay_email')->inactive(0);

        $self->field('credit_card_number')->inactive(1);
        $self->field('credit_card_exp_month')->inactive(1);
        $self->field('credit_card_exp_year')->inactive(1);
        $self->field('credit_card_cvv')->inactive(1);

    }

};

has '+item_class' => ( default => 'Cart' );

has_field 'payment_type'        => ( 
    type                => 'Text', 
    label               => 'Payment Type', 
    required            => 1,
    required_message    => 'You must Select a Payment type', 
    css_class           => 'form_col_a',
);
has_field 'payment_amount'        => (
    type                => 'Hidden',
    label               => 'Payment Amount',
    required            => 1,
    required_message    => 'You must enter a payment amount',
    css_class           => 'form_col_a',
);
has_field 'credit_card_number'        => (
    type                => 'Text',
    label               => 'Credit Card #',
    required            => 1,
    required_message    => 'You must enter a credit card number',
    css_class           => 'form_col_a',
);
has_field 'credit_card_exp_month' => (
    type                => 'Select',
    label               => '',
    required            => 1,
    required_message    => 'You must select an expiration month',
    css_class           => 'form_col_b',
    options             => [
                            { value => '01', label => '01'},
                            { value => '02', label => '02'},
                            { value => '03', label => '03'},
                            { value => '04', label => '04'},
                            { value => '05', label => '05'},
                            { value => '06', label => '06'},
                            { value => '07', label => '07'},
                            { value => '08', label => '08'},
                            { value => '09', label => '09'},
                            { value => '10', label => '10'},
                            { value => '11', label => '11'},
                            { value => '12', label => '12'},
                            ]
);
has_field 'credit_card_exp_year' => (
    type                => 'Select',
    label               => '',
    required            => 1,
    required_message    => 'You must select an expiration year',
    css_class           => 'form_col_b',
    options             => [
                            { value => '09', label => '2009'},
                            { value => '10', label => '2010'},
                            { value => '11', label => '2011'},
                            { value => '12', label => '2012'},
                            { value => '13', label => '2013'},
                            { value => '14', label => '2014'},
                            { value => '15', label => '2015'},
                            { value => '16', label => '2016'},
                            { value => '17', label => '2017'},
                            { value => '18', label => '2018'},
                            ]
);

has_field 'credit_card_cvv'        => (
    type                => 'Text',
    label               => 'CVV',
    required            => 1,
    required_message    => 'You must enter a credit card cvv',
    css_class           => 'form_col_a',
);
has_field 'paypal_email'        => (
    type                => 'Text',
    label               => 'Paypal Email',
    css_class           => 'form_col_a',
);



has_field 'submit'       => ( type => 'Submit', value => 'Save' );

no HTML::FormHandler::Moose;
1;
